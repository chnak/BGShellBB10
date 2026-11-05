/*
 * bgqrcam - the QR scanner helper.
 *
 * The camera on a BlackBerry 10 device is only reachable through the Cascades
 * multimedia API (bb::cascades::multimedia::Camera), and Cascades controls
 * only exist inside a Cascades application - creating one in the plain Qt
 * Widgets program BGShellBB10 aborts the process.  So the camera lives here,
 * in its own process: a Cascades application with no window of its own.
 *
 * Frames are pulled straight out of the camera into buffers we own, so no
 * viewfinder widget is needed.
 *
 * Why the decoder is ZXing and not bb::cascades::multimedia::BarcodeDetector:
 * BarcodeDetector scans every preview frame the camera produces.  On this
 * hardware that is a full 1024x768 pass thirty times a second, which keeps
 * both cores busy and makes the application on the other end of the pipe
 * stutter.  Decoding happens here on a timer instead, at a rate we choose,
 * on a downscaled copy - the camera can run as fast as it likes.
 *
 * Two streams go back to the caller:
 *   stdout  - text, one message per line:
 *               QRCODE:<the scanned text>   the result
 *               LOG:<a diagnostic line>      progress / why it gave up
 *   stderr  - the live picture, as a small grayscale image, so the caller
 *             can show what the camera sees:
 *               'F' 'R' 'M' <w:16> <h:16> <reserved> <w*h bytes>
 */
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include <cstdio>

#include <bb/cascades/Application>
#include <bb/cascades/multimedia/Camera>
#include <bb/cascades/multimedia/CameraError>
#include <bb/cascades/multimedia/CameraUnit>

#include "zxingzx.h"

// How many preview buffers to keep in the camera pool.
const int kPreviewBuffers = 3;

// Give up after this long so a stuck camera cannot hang the caller.
const int kTimeoutMs = 120000;

// The picture sent back to the caller is this wide; the height follows the
// frame's aspect ratio.  It only has to be good enough to aim with, and
// every pixel crosses a pipe, so it stays small.
const int kPreviewWidth = 120;
const int kPreviewIntervalMs = 400;

// Decoding is the expensive part.  A QR code does not move, so a few
// attempts per second are plenty - and this is what keeps the rest of the
// system responsive.
const int kDecodeIntervalMs = 350;

// The frame is downscaled to this width before decoding.  A code that fills
// a reasonable part of the viewfinder still has plenty of pixels left.
const int kDecodeWidth = 512;

static void logLine(const QString &text)
{
	printf("LOG:%s\n", qPrintable(text));
	fflush(stdout);
}

class Scanner : public QObject
{
	Q_OBJECT
public:
	Scanner()
	    : _camera(0), _timer(0), _haveFrame(false),
	      _frameW(0), _frameH(0), _frameStride(0),
	      _gray(0), _grayW(0), _grayH(0),
	      _reader(0), _lastPreview(0), _done(false)
	{}

	int start()
	{
		// The Cascades camera wants a Container as its parent.  There is no
		// scene in this program, so it is created without one.
		_camera = new bb::cascades::multimedia::Camera(0);

		connect(_camera,
		        SIGNAL(previewFrameAvailable(bb::cascades::multimedia::SharedUCharPointer,quint64,uint,uint,uint)),
		        this,
		        SLOT(onPreviewFrame(bb::cascades::multimedia::SharedUCharPointer,quint64,uint,uint,uint)));
		connect(_camera,
		        SIGNAL(viewfinderStartFailed(bb::cascades::multimedia::CameraError::Type)),
		        this,
		        SLOT(onFailed(bb::cascades::multimedia::CameraError::Type)));
		connect(_camera, SIGNAL(cameraOpened()), this, SLOT(onCameraOpened()));
		connect(_camera,
		        SIGNAL(cameraOpenFailed(bb::cascades::multimedia::CameraError::Type)),
		        this,
		        SLOT(onOpenFailed(bb::cascades::multimedia::CameraError::Type)));

		if (!_camera->isCameraAccessible(bb::cascades::multimedia::CameraUnit::Rear)) {
			logLine(QString::fromUtf8("没有可用的后置摄像头"));
			return 2;
		}

		// open() is asynchronous: the camera is only really open once
		// cameraOpened() arrives, and previewBufferSize() answers 0 until
		// then.  Everything below therefore happens in onCameraOpened().
		logLine(QString::fromUtf8("正在打开摄像头…"));
		_camera->open(bb::cascades::multimedia::CameraUnit::Rear);
		return 0;
	}

public slots:
	// The camera is open now - it can tell us how big a preview buffer has
	// to be, and it will start handing out frames once the buffers are in.
	void onCameraOpened()
	{
		const quint64 bufSize = _camera->previewBufferSize();
		if (bufSize == 0) {
			logLine(QString::fromUtf8("摄像头没有提供预览缓冲"));
			QCoreApplication::exit(3);
			return;
		}

		for (int i = 0; i < kPreviewBuffers; ++i) {
			QSharedPointer<unsigned char> buf(new unsigned char[bufSize]);
			_camera->addPreviewBuffer(buf, bufSize);
		}

		_camera->startViewfinder();

		// The decode clock.  It does nothing until a frame has arrived.
		_timer = new QTimer(this);
		_timer->setInterval(kDecodeIntervalMs);
		connect(_timer, SIGNAL(timeout()), this, SLOT(onTick()));
		_timer->start();

		logLine(QString::fromUtf8("取景器已启动"));
	}

	void onOpenFailed(bb::cascades::multimedia::CameraError::Type error)
	{
		logLine(QString::fromUtf8("打开摄像头失败 (%1)").arg((int)error));
		QCoreApplication::exit(5);
	}

	void onFailed(bb::cascades::multimedia::CameraError::Type error)
	{
		logLine(QString::fromUtf8("取景器启动失败 (%1)").arg((int)error));
		_done = true;
		QCoreApplication::exit(4);
	}

	// The camera hands out NV12 frames.  Only the luma plane is used: it is
	// already what both the preview and the decoder want, so no colour
	// conversion ever happens.
	void onPreviewFrame(const bb::cascades::multimedia::SharedUCharPointer &buffer,
	                    quint64 size, unsigned int width, unsigned int height,
	                    unsigned int stride)
	{
		if (!buffer || width == 0 || height == 0 || stride < width)
			return;

		const quint64 yBytes = (quint64)stride * height;
		if (size < yBytes)
			return;

		const unsigned char *src = buffer.data();
		if (!src)
			return;

		_haveFrame   = true;
		_frameW      = width;
		_frameH      = height;
		_frameStride = stride;
		_frame       = QByteArray((const char *)src, (int)yBytes);
	}

	// Runs a few times a second: send a small picture to the caller, and
	// try to decode the newest frame.
	void onTick()
	{
		if (_done || !_haveFrame)
			return;

		const qint64 now = QDateTime::currentMSecsSinceEpoch();

		if (now - _lastPreview >= kPreviewIntervalMs) {
			_lastPreview = now;
			sendPreview();
		}

		decodeLatest();
	}

private:
	void sendPreview()
	{
		if (_frame.isEmpty() || _frameW == 0 || _frameH == 0)
			return;

		const unsigned int step = _frameW / kPreviewWidth > 0
		                        ? _frameW / kPreviewWidth : 1;
		const unsigned int outW = _frameW / step;
		const unsigned int outH = _frameH / step;
		if (outW == 0 || outH == 0)
			return;

		// The header is exactly 8 bytes so the reader can find the next
		// frame without guessing.
		QByteArray pic;
		pic.reserve(8 + outW * outH);
		pic.append('F');
		pic.append('R');
		pic.append('M');
		pic.append((char)(outW & 0xff));
		pic.append((char)((outW >> 8) & 0xff));
		pic.append((char)(outH & 0xff));
		pic.append((char)((outH >> 8) & 0xff));
		pic.append((char)0);
		for (unsigned int y = 0; y < _frameH; y += step) {
			const char *row = _frame.constData() + (quint64)y * _frameStride;
			for (unsigned int x = 0; x < _frameW; x += step)
				pic.append(row[x]);
		}

		fwrite(pic.constData(), 1, pic.size(), stderr);
		fflush(stderr);
	}

	void decodeLatest()
	{
		if (_frameW == 0 || _frameH == 0)
			return;

		// Downscale before decoding: a full 1024x768 pass is the difference
		// between a responsive phone and a frozen one.
		const unsigned int step = _frameW / kDecodeWidth > 0
		                        ? _frameW / kDecodeWidth : 1;
		const int w = (int)(_frameW / step);
		const int h = (int)(_frameH / step);
		if (w <= 0 || h <= 0)
			return;

		if (_grayW != w || _grayH != h) {
			delete[] _gray;
			_grayW = w;
			_grayH = h;
			_gray = new unsigned char[w * h];
			_reader = 0;   // the luminance source points at _gray
		}

		for (int y = 0; y < h; ++y) {
			const char *row = _frame.constData() + (quint64)(y * step) * _frameStride;
			unsigned char *dst = _gray + y * w;
			for (int x = 0; x < w; ++x)
				dst[x] = (unsigned char)row[x * step];
		}

		if (!_reader) {
			_source   = zxing::Ref<zxing::LuminanceSource>(
			                new zxing::GreyscaleLuminanceSource(_gray, w, h,
			                                                    0, 0, w, h));
			_binarizer = zxing::Ref<zxing::Binarizer>(
			                new zxing::HybridBinarizer(_source));
			_bitmap    = zxing::Ref<zxing::BinaryBitmap>(
			                new zxing::BinaryBitmap(_binarizer));
			_reader    = new zxing::MultiFormatReader();
		}

		zxing::Ref<zxing::Result> result = _reader->decode(_bitmap);
		if (result.isNull())
			return;

		zxing::Ref<zxing::String> str = result->getText();
		if (str.isNull())
			return;

		const char *chars = zxingStringChars(str.get());
		if (!chars || !*chars)
			return;

		finish(QString::fromUtf8(chars));
	}

	void finish(const QString &text)
	{
		if (_done || text.isEmpty())
			return;
		_done = true;

		printf("QRCODE:%s\n", qPrintable(text));
		fflush(stdout);

		QCoreApplication::exit(0);
	}

	bb::cascades::multimedia::Camera *_camera;
	QTimer                          *_timer;

	QByteArray   _frame;
	bool         _haveFrame;
	unsigned int _frameW, _frameH, _frameStride;

	unsigned char *_gray;
	int            _grayW, _grayH;

	zxing::Ref<zxing::LuminanceSource> _source;
	zxing::Ref<zxing::Binarizer>       _binarizer;
	zxing::Ref<zxing::BinaryBitmap>    _bitmap;
	zxing::MultiFormatReader          *_reader;

	qint64 _lastPreview;
	bool   _done;
};

int main(int argc, char **argv)
{
	bb::cascades::Application app(argc, argv);

	Scanner scanner;
	const int rc = scanner.start();
	if (rc != 0)
		return rc;

	QTimer::singleShot(kTimeoutMs, &app, SLOT(quit()));

	return app.exec();
}

#include "bgqrcam.moc"
