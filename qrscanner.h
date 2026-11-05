/*
 * QR code scanning for BGShellBB10.
 *
 * The camera on a BlackBerry 10 device is only reachable through the Cascades
 * multimedia API, and Cascades controls only exist inside a Cascades
 * application: bb::cascades::Application derives from QCoreApplication, not
 * from QApplication, and creating even a bb::cascades::Container in this
 * plain Qt Widgets program aborts the process.
 *
 * So the camera lives in a helper process - bgqrcam, a Cascades application
 * with no window of its own that pulls the preview frames straight into
 * buffers and lets the barcode detector do the work.  It prints the decoded
 * text on stdout and exits; this overlay starts it, reads that line and
 * copies the text to the clipboard.
 *
 * A decoded code is copied to the clipboard as soon as it is read and shown
 * in a result card, from where it can also be pushed into the running shell.
 */
#ifndef QRSCANNER_H
#define QRSCANNER_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

class QProcess;

// The live picture the helper sends back, drawn as a plain widget.
class QrPreview : public QWidget
{
public:
	explicit QrPreview(QWidget *parent = 0);

	void setFrame(const QImage &image);
	void clear();

protected:
	void paintEvent(QPaintEvent *event);

private:
	QImage _image;
};

class CMyQrScanner : public QWidget
{
	Q_OBJECT

public:
	explicit CMyQrScanner(QWidget *parent = 0);
	~CMyQrScanner();

	void popup();
	void dismiss();
	bool isOpen() const { return isVisible(); }

signals:
	// The decoded text.  It has already been copied to the clipboard.
	void scanned(const QString &text);
	// The user asked for the text to be typed into the running shell.
	void insertRequested(const QString &text);

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void keyPressEvent(QKeyEvent *event);

private slots:
	void onHelperOutput();
	void onHelperError();
	void onHelperFinished(int exitCode, int exitStatus);
	void onCopy();
	void onInsert();
	void onClose();

private:
	void buildUi();
	void startHelper();
	void stopHelper();
	void showResult(const QString &text);
	void setStatus(const QString &text);

	QProcess    *_helper;
	QByteArray  _helperOut;   // stdout: text lines
	QByteArray  _helperErr;   // stderr: the picture

	QLabel      *_status;
	QrPreview   *_preview;
	QLabel      *_result;
	QToolButton *_btnCopy;
	QToolButton *_btnInsert;
	QToolButton *_btnClose;

	QString _lastText;
	bool    _haveResult;
	bool    _viaWrapper;   // running through /base/bin/g_camera
};

#endif // QRSCANNER_H
