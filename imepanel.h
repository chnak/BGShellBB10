/*
 * The IME candidate strip for BGShellBB10.
 *
 * A terminal cannot use a system input panel (the shell needs the raw UTF-8
 * bytes, and BB10's Qt4 has no IME plugin interface a full screen widget can
 * drive), so the candidate list is drawn by the terminal itself as a strip
 * along the bottom edge:
 *
 *   [中]  ni   1 你  2 尼  3 泥  4 呢      1/6  < >
 *
 * Every candidate can be picked either with the number key printed in front
 * of it or by tapping it (Q30 / Passport / Classic have a touchscreen); the
 * arrows page through the list. The strip never resizes the terminal - it is
 * just an overlay, like the SYM grid.
 *
 * On a Q20 / Q10 / Q5 / Classic the trackpad drives the strip: left-right
 * steps the highlight (the frame drawn around one candidate), up-down changes
 * the page, and a trackpad click commits the highlighted candidate.
 */
#ifndef IMEPANEL_H
#define IMEPANEL_H

#include <QtCore/QList>
#include <QtCore/QRect>
#include <QtCore/QStringList>
#include <QtCore/QTime>
#include <QtGui/QFont>
#include <QtGui/QWidget>

class CImePanel : public QWidget
{
	Q_OBJECT

public:
	explicit CImePanel(QWidget *parent = 0);

	bool isOpen() const { return isVisible(); }

	void setCjkFontFamily(const QString &family);

	// buffer = what has been typed so far, candidates = the current page,
	// pageIndex is 0 based, pageCount tells whether the < > arrows are live
	void setState(const QString &buffer, const QStringList &candidates,
	              int pageIndex, int pageCount);

	void setBottomOffset(int px);

	// Show / hide the strip.  Both are called by the terminal when a
	// composition starts or ends.
	void popup();
	void dismiss();

	// Height the terminal has to keep free below its last row, so the strip
	// sits *under* the text instead of on top of the cursor line.
	int stripHeight() const;

	// Trackpad / arrow key driving. delta = +1 next, -1 previous; the step
	// wraps around the candidates that are actually on screen and is paced
	// like a key repeat, because the trackpad streams events.
	void moveHighlight(int delta);
	int  highlight() const { return _highlight; }
	void commitHighlighted();      // emits candidateChosen(highlight)
	bool hasHighlight() const;     // is there a candidate Space / tap can take

signals:
	void candidateChosen(int index);   // 0 based, into the current page
	void pageRequested(int delta);   // -1 = previous page, +1 = next page

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	// Same trap as the key strip: the second tap of a fast double tap arrives
	// as a DblClick, so without this every other candidate tap is swallowed.
	void mouseDoubleClickEvent(QMouseEvent *event);

private:
	void relayout();
	bool idle() const { return _buffer.isEmpty() && _candidates.isEmpty(); }

	QString     _buffer;
	QStringList _candidates;
	int         _page;         // 0 based index of the page on screen
	int         _pages;        // how many pages the engine has for this word
	int         _highlight;    // candidate the frame is drawn around
	int         _bottomOffset; // pixels taken below by the key strip
	QTime       _lastStep;    // pacing for moveHighlight()

	QList<QRect> _candidateRects;
	QRect        _prevRect;
	QRect        _nextRect;

	QFont _glyphFont;   // candidate characters
	QFont _smallFont;   // numbers and pinyin
};

#endif // IMEPANEL_H
