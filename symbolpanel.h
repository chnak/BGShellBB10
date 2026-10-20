/*
 * Term50's "main_symmenu" overlay, rebuilt as a real Qt widget.
 *
 * Pressing the physical SYM key pops up a grid of symbols. Every cell shows
 * the glyph with the key it is bound to in the corner, exactly like Term50's
 * symmenu renderer (symmenu_render.c: big glyph + small corner letter). A
 * symbol can be picked either by tapping its cell (Q30/Passport/Classic have
 * a touchscreen) or by pressing that key, and SYM/Esc again closes the grid.
 *
 * The grid contents are Term50's DEFAULT_SYMMENU_ENTRIES
 * (preferences.c, 2 rows of 10 / 9) so the two apps stay in sync.
 */
#ifndef SYMBOLPANEL_H
#define SYMBOLPANEL_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>

class QLabel;

class CSymbolPanel : public QWidget
{
	Q_OBJECT

public:
	explicit CSymbolPanel(QWidget *parent = 0);

	// Term50 main_symmenu lookup (Qt::Key_A..Key_P style keys)
	static bool    hasKey(int key);
	static QString textForKey(int key);

	void popup();                       // place over the bottom of the parent
	void dismiss();
	bool isOpen() const { return isVisible(); }

signals:
	void symbolChosen(const QString &text);   // utf8 encoded by the display

protected:
	void paintEvent(QPaintEvent *event);
	bool eventFilter(QObject *watched, QEvent *event);

private:
	void buildCells();
	void relayout();

	QList<QLabel *> _cells;    // one label per grid cell, row-major
	QList<QString>  _texts;    // symbol inserted by that cell
	QList<int>      _rowLens;  // cells per row (Term50 row lengths)
	QList<int>      _keyHints; // source key per cell, 0 when none
	int             _cellHeight;
	int             _margin;
};

#endif // SYMBOLPANEL_H
