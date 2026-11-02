/*
 * CSessionTabs - Chrome-style tab strip for terminal sessions.
 *
 * The strip is drawn by hand instead of using QTabWidget: BB10 has no
 * hover, the tab must be tappable with a thumb, and the active tab should
 * melt into the terminal below it (same background, accent hairline on top)
 * the way a browser does.
 *
 * Each tab hosts one QTermWidget; pages are plain child widgets stacked at
 * (0, barHeight).  The strip scrolls horizontally with a finger when the
 * tabs no longer fit.
 */
#ifndef CSESSIONTABS_H
#define CSESSIONTABS_H

#include <QWidget>
#include <QStringList>
#include <QList>

class CSessionTabs : public QWidget
{
	Q_OBJECT

public:
	explicit CSessionTabs(QWidget *parent = 0);

	void addPage(QWidget *page, const QString &title);
	void removePage(int index);

	int  count() const { return _pages.size(); }
	int  currentIndex() const { return _current; }
	QWidget *currentPage() const;
	QWidget *pageAt(int index) const;

	void setCurrentIndex(int index);
	void setPageTitle(int index, const QString &title);
	QString pageTitle(int index) const;

	/** Forget cached text metrics (UI font changed in the settings page). */
	void refreshMetrics();

	int  barHeight() const { return _barH; }

	/** Space the pages may use (below the strip). */
	QRect contentGeometry() const;

signals:
	void newSessionRequested();
	void currentChanged(int index);
	void closeRequested(int index);
	/** The gear at the right end of the strip was tapped. */
	void settingsRequested();

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	int tabWidth() const;
	int totalWidth() const;
	int maxScroll() const;
	/** Width the tabs may use: everything left of the "+" and the gear. */
	int viewWidth() const;
	int indexAt(const QPoint &pos, bool *closeHit) const;
	bool plusHit(const QPoint &pos) const;
	bool gearHit(const QPoint &pos) const;
	void layoutPages();

	QList<QWidget *> _pages;
	QStringList      _titles;
	int              _current;
	int              _barH;
	int              _scroll;
	mutable int      _tabW;   // cached tab width, -1 = recompute

	bool _dragging;
	bool _moved;
	int  _pressX;
	int  _pressScroll;
	int  _pressIndex;
	bool _pressClose;
	bool _pressPlus;
	bool _pressGear;
};

#endif // CSESSIONTABS_H
