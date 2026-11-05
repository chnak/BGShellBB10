/*
 * QR code scanning - see qrscanner.h.
 */
#include <QtCore/QDebug>
#include <cstring>
#include <unistd.h>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include "qrscanner.h"
#include "uitheme.h"

namespace {

// Where the helper lives.  It ships inside the bar next to the ssh suite.
const char *kHelperName = "bgqrcam";

// The camera service only lets the camera group open /dev/camera/*, and
// /base/bin/g_camera is the setgid wrapper that runs a command in that
// group.  It reads the command from stdin, so the helper is started
// through it:
//
//     echo "/path/to/bgqrcam" | g_camera
//
// That way the scanner works without the application needing the camera
// permission in its bar descriptor - which matters, because the permission
// is only granted when the bar is installed.
const char *kCameraWrapper = "/base/bin/g_camera";

QString helperPath()
{
	const QString appDir = QCoreApplication::applicationDirPath();
	const QStringList candidates = QStringList()
	    << appDir + "/bin/" + kHelperName
	    << appDir + "/" + kHelperName
	    << QString("/accounts/1000/appdata/com.q20.shell.testDev_m_q20_shelld9a3d1c4/app/native/bin/") + kHelperName;

	for (int i = 0; i < candidates.size(); ++i) {
		if (QFile::exists(candidates.at(i)))
			return candidates.at(i);
	}
	return QString();
}

} // namespace

CMyQrScanner::CMyQrScanner(QWidget *parent) :
	QWidget(parent),
	_helper(0),
	_status(0),
	_preview(0),
	_result(0),
	_btnCopy(0),
	_btnInsert(0),
	_btnClose(0),
	_haveResult(false),
	_viaWrapper(false)
{
	buildUi();
}

// ---------------------------------------------------------------- preview
QrPreview::QrPreview(QWidget *parent) :
	QWidget(parent)
{
	setAutoFillBackground(false);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void QrPreview::setFrame(const QImage &image)
{
	_image = image;
	update();
}

void QrPreview::clear()
{
	_image = QImage();
	update();
}

void QrPreview::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(0x05, 0x07, 0x09));

	if (_image.isNull())
		return;

	// Fit the picture into the widget, keeping the aspect ratio.
	const int dw = width(), dh = height();
	const int iw = _image.width(), ih = _image.height();
	int nw = dw, nh = dh;
	if (iw > 0 && ih > 0) {
		nh = (int)((qint64)dw * ih / iw);
		if (nh > dh) {
			nh = dh;
			nw = (int)((qint64)dh * iw / ih);
		}
	}
	p.drawImage(QRect((dw - nw) / 2, (dh - nh) / 2, nw, nh), _image);
}

CMyQrScanner::~CMyQrScanner()
{
	stopHelper();
}

void CMyQrScanner::buildUi()
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAutoFillBackground(false);
	setFocusPolicy(Qt::StrongFocus);

	_preview = new QrPreview(this);
	_preview->setObjectName(QString::fromLatin1("preview"));

	_status = new QLabel(this);
	_status->setObjectName(QString::fromLatin1("status"));
	_status->setAlignment(Qt::AlignCenter);
	_status->setWordWrap(true);
	_status->setText(QString::fromUtf8("正在启动相机…"));

	_result = new QLabel(this);
	_result->setObjectName(QString::fromLatin1("result"));
	_result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	_result->setWordWrap(true);
	_result->hide();

	_btnCopy = new QToolButton(this);
	_btnCopy->setObjectName(QString::fromLatin1("primary"));
	_btnCopy->setText(QString::fromUtf8("复制"));
	_btnCopy->setFocusPolicy(Qt::NoFocus);

	_btnInsert = new QToolButton(this);
	_btnInsert->setObjectName(QString::fromLatin1("secondary"));
	_btnInsert->setText(QString::fromUtf8("插入终端"));
	_btnInsert->setFocusPolicy(Qt::NoFocus);

	_btnClose = new QToolButton(this);
	_btnClose->setObjectName(QString::fromLatin1("secondary"));
	_btnClose->setText(QString::fromUtf8("关闭"));
	_btnClose->setFocusPolicy(Qt::NoFocus);

	connect(_btnCopy,   SIGNAL(clicked()), this, SLOT(onCopy()));
	connect(_btnInsert, SIGNAL(clicked()), this, SLOT(onInsert()));
	connect(_btnClose,  SIGNAL(clicked()), this, SLOT(onClose()));
}

void CMyQrScanner::resizeEvent(QResizeEvent *e)
{
	QWidget::resizeEvent(e);

	const int margin = 18;
	const int hintH  = 56;
	const int btnH   = 64;

	const int cardH = 190;
	const int cardY = qMax(0, height() - cardH);

	// The picture fills everything above the hint line.
	const int viewH = qMax(120, height() - hintH - 2 * margin);
	_preview->setGeometry(margin, margin, width() - 2 * margin, viewH);
	_status->setGeometry(margin, margin + viewH, width() - 2 * margin, hintH);
	_result->setGeometry(margin + 8, cardY + 52, width() - 2 * margin - 16, 74);

	const int bw = (width() - 3 * margin) / 3;
	const int by = height() - btnH - margin / 2;
	_btnCopy  ->setGeometry(margin,                     by, bw, btnH);
	_btnInsert->setGeometry(margin + bw + margin / 2,   by, bw, btnH);
	_btnClose ->setGeometry(margin + 2 * (bw + margin/2), by, bw, btnH);
}

void CMyQrScanner::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), QColor(0x0b, 0x0e, 0x11));

	if (_haveResult) {
		const int margin = 18;
		const int cardH  = 190;
		const int cardY  = qMax(0, height() - cardH);
		const QRect card(margin, cardY, width() - 2 * margin, cardH);

		p.setPen(Qt::NoPen);
		p.setBrush(UiTheme::withAlpha(UiTheme::Surface, 240));
		p.drawRoundedRect(card, 14, 14);
		p.setPen(QPen(UiTheme::Border, 1));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(card, 14, 14);

		p.setPen(UiTheme::Accent);
		p.setFont(UiTheme::monoFont(20, true));
		p.drawText(QRect(card.left() + 14, card.top() + 10,
		                 card.width() - 28, 30),
		           Qt::AlignLeft | Qt::AlignVCenter,
		           QString::fromUtf8("已复制到剪贴板"));
	}
}

void CMyQrScanner::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		dismiss();
		return;
	}
	QWidget::keyPressEvent(event);
}

void CMyQrScanner::popup()
{
	if (!parentWidget())
		return;

	setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
	_haveResult = false;
	_viaWrapper = false;
	_lastText.clear();
	_helperOut.clear();
	_helperErr.clear();
	_preview->clear();
	_result->hide();
	setStatus(QString::fromUtf8("正在启动相机…"));

	show();
	raise();
	setFocus();

	startHelper();
}

void CMyQrScanner::dismiss()
{
	stopHelper();
	hide();
}

void CMyQrScanner::startHelper()
{
	const QString path = helperPath();
	if (path.isEmpty()) {
		setStatus(QString::fromUtf8("找不到扫码程序 bgqrcam"));
		return;
	}

	qDebug("QR: starting helper %s", qPrintable(path));

	if (!_helper) {
		_helper = new QProcess(this);
		connect(_helper, SIGNAL(readyReadStandardOutput()),
		        this, SLOT(onHelperOutput()));
		connect(_helper, SIGNAL(readyReadStandardError()),
		        this, SLOT(onHelperError()));
		connect(_helper, SIGNAL(finished(int,QProcess::ExitStatus)),
		        this, SLOT(onHelperFinished(int,QProcess::ExitStatus)));
	}

	// The application carries the camera permission, and a child it starts
	// inherits that, so the helper is normally just run directly.  When that
	// is refused the setgid camera wrapper is the fallback, see
	// onHelperFinished() - it costs a copy to /tmp and a shell, so it is
	// only paid for when it is needed.
	if (!_viaWrapper) {
		_helper->start(path, QStringList());
		if (!_helper->waitForStarted(5000)) {
			setStatus(QString::fromUtf8("扫码程序启动失败"));
			return;
		}
		qDebug("QR: helper started directly");
		setStatus(QString::fromUtf8("把二维码放进取景框，扫到后会自动复制"));
		return;
	}

	// Fallback: /base/bin/g_camera is setgid to the camera group.  Two things
	// have to be handled for it:
	//
	//  - it is setgid, so it scrubs LD_LIBRARY_PATH; the library path
	//    therefore has to be part of the command itself, and it is
	//    deliberately a minimal one - the application's own path holds the
	//    ssh suite's libz and libcrypto, which break the helper's loader.
	//  - the application's directory carries an ACL that only lets the
	//    application's user in, so the camera group cannot execute anything
	//    from there ("cannot execute - Permission denied").  The helper is
	//    copied somewhere everyone can reach first.
	QString runPath = path;
	const QString tmpPath = QString("/tmp/bgqrcam.%1").arg((int)getpid());
	QFile::remove(tmpPath);
	if (QFile::copy(path, tmpPath)) {
		QFile::setPermissions(tmpPath,
		    QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
		    QFile::ReadUser  | QFile::ExeUser  |
		    QFile::ReadOther | QFile::ExeOther);
		runPath = tmpPath;
	}

	_helper->start(kCameraWrapper, QStringList());
	if (!_helper->waitForStarted(5000)) {
		setStatus(QString::fromUtf8("扫码程序启动失败"));
		return;
	}
	QString cmd = QString("LD_LIBRARY_PATH=/usr/lib/qt4/lib:/usr/lib ") + runPath;
	_helper->write(cmd.toAscii());
	_helper->write("\n");
	_helper->closeWriteChannel();
	qDebug("QR: helper started through %s", kCameraWrapper);
	setStatus(QString::fromUtf8("把二维码放进取景框，扫到后会自动复制"));
}

void CMyQrScanner::stopHelper()
{
	if (!_helper)
		return;

	if (_helper->state() != QProcess::NotRunning) {
		_helper->terminate();
		if (!_helper->waitForFinished(1500))
			_helper->kill();
	}
	_helperOut.clear();
}

void CMyQrScanner::onHelperOutput()
{
	if (!_helper)
		return;

	_helperOut += _helper->readAllStandardOutput();

	// The helper writes one message per line.
	int nl;
	while ((nl = _helperOut.indexOf('\n')) >= 0) {
		const QByteArray line = _helperOut.left(nl);
		_helperOut.remove(0, nl + 1);

		if (line.startsWith("LOG:")) {
			qDebug("QR: helper: %s", line.mid(4).constData());
			continue;
		}

		if (!line.startsWith("QRCODE:"))
			continue;

		const QString text = QString::fromUtf8(line.mid(7));
		if (text.isEmpty())
			continue;

		_lastText = text;
		_haveResult = true;

		QApplication::clipboard()->setText(text);
		showResult(text);
		emit scanned(text);
		setStatus(QString::fromUtf8("已复制到剪贴板"));
		update();
	}
}

// The helper sends the live picture on stderr, as frames of
//   'F' 'R' 'M' <w:16> <h:16> <reserved> <w*h gray bytes>
// Everything that is not a frame is text from the shell or the loader - it
// is reported once, and never confused with picture data.
void CMyQrScanner::onHelperError()
{
	if (!_helper)
		return;

	_helperErr += _helper->readAllStandardError();

	// A frame is at most a 320x240 picture; anything beyond a few of those
	// means the stream is out of step, so drop it instead of growing.
	const int kMaxFrame = 8 + 320 * 240;

	while (_helperErr.size() >= 8) {
		if (_helperErr.left(3) != QByteArray("FRM", 3)) {
			// Text.  Take one line at a time and show it - that is where the
			// dynamic loader and the shell report why they gave up.
			const int nl = _helperErr.indexOf('\n');
			if (nl < 0) {
				if (_helperErr.size() > 512)
					_helperErr.clear();   // runaway text, drop it
				break;
			}

			QByteArray line = _helperErr.left(nl).trimmed();
			_helperErr.remove(0, nl + 1);

			// Only printable text is worth logging: if this was picture
			// data that lost its header, printing it would flood the log
			// and stall the UI thread.
			bool printable = !line.isEmpty();
			for (int i = 0; i < line.size() && printable; ++i) {
				const unsigned char c = (unsigned char)line[i];
				if (c < 32 || c > 126)
					printable = false;
			}
			if (printable)
				qWarning("QR: helper: %s", line.constData());
			continue;
		}

		const int w = (unsigned char)_helperErr[3] | ((unsigned char)_helperErr[4] << 8);
		const int h = (unsigned char)_helperErr[5] | ((unsigned char)_helperErr[6] << 8);
		if (w <= 0 || h <= 0 || w * h > kMaxFrame) {
			_helperErr.clear();   // out of step: start over at the next frame
			break;
		}

		const int need = 8 + w * h;
		if (_helperErr.size() < need)
			break;

		QImage img(w, h, QImage::Format_RGB32);
		if (!img.isNull()) {
			// Qt 4 has no grayscale format; expand the bytes ourselves.
			const unsigned char *src = (const unsigned char *)_helperErr.constData() + 8;
			QRgb *dst = (QRgb *)img.scanLine(0);
			for (int i = 0; i < w * h; ++i)
				dst[i] = qRgb(src[i], src[i], src[i]);
			_preview->setFrame(img);
		}
		_helperErr.remove(0, need);
	}
}

void CMyQrScanner::onHelperFinished(int exitCode, int exitStatus)
{
	qDebug("QR: helper finished, exit %d status %d", exitCode, exitStatus);

	if (!_haveResult) {
		// "no camera" while running directly means this install did not give
		// the application the camera permission.  The setgid wrapper can
		// still reach it, so try that once before giving up.
		if (exitCode == 2 && !_viaWrapper && isVisible()
		    && QFile::exists(kCameraWrapper)) {
			qDebug("QR: retrying through %s", kCameraWrapper);
			_viaWrapper = true;
			_helperOut.clear();
			_helperErr.clear();
			startHelper();
			return;
		}

		if (exitCode == 0)
			setStatus(QString::fromUtf8("没有扫到二维码"));
		else if (exitCode == 2)
			setStatus(QString::fromUtf8("这台设备没有可用的摄像头"));
		else if (exitCode == 3)
			setStatus(QString::fromUtf8("摄像头没有提供预览画面"));
		else
			setStatus(QString::fromUtf8("扫码程序出错（%1）").arg(exitCode));
	}
}

void CMyQrScanner::showResult(const QString &text)
{
	const int margin = 18;
	const int cardH  = 190;
	const int cardY  = qMax(0, height() - cardH);
	const QRect box(margin + 8, cardY + 52, width() - 2 * margin - 16, 74);

	QFont f = UiTheme::monoFont(20);
	const QFontMetrics fm(f);
	_result->setFont(f);
	_result->setText(fm.elidedText(text, Qt::ElideMiddle, qMax(40, box.width())));
	_result->setToolTip(text);
	_result->show();
}

void CMyQrScanner::setStatus(const QString &text)
{
	_status->setText(text);
}

void CMyQrScanner::onCopy()
{
	if (_lastText.isEmpty())
		return;
	QApplication::clipboard()->setText(_lastText);
	setStatus(QString::fromUtf8("已复制到剪贴板"));
}

void CMyQrScanner::onInsert()
{
	if (_lastText.isEmpty())
		return;
	emit insertRequested(_lastText);
	dismiss();
}

void CMyQrScanner::onClose()
{
	dismiss();
}
