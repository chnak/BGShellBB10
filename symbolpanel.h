/*
 * The SYM panel: Term50's "main_symmenu" overlay, plus the custom key strip.
 *
 * Pressing the physical SYM key pops this up over the bottom of the terminal.
 * It holds two things:
 *
 *   1. Term50's symbol grid (2 rows of 10 / 9 cells, glyph + corner key hint),
 *      kept verbatim from preferences.c so both apps show the same symbols.
 *   2. The user defined control keys - the same list the bottom key strip
 *      edits in Settings (CKeyBar::loadItems), so ^B / ^C / arrows / a whole
 *      "git status\r" are one SYM press away without a permanently reserved
 *      strip of screen.
 *
 * A symbol can be picked by tapping its cell (Q30/Passport/Classic) or by
 * pressing its bound key; SYM or Esc closes the panel. Control keys send raw
 * pty bytes; the ones you press repeatedly (Tab, arrows, PgUp/PgDn, Backspace)
 * keep the panel open, the rest close it after firing.
 */
#ifndef SYMBOLPANEL_H
#define SYMBOLPANEL_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>

#include "keybar.h"

class QLabel;

class CSymbolPanel : public QWidget
{
	Q_OBJECT

public:
	explicit CSymbolPanel(QWidget *parent = 0);

	// Term50 main_symmenu lookup (Qt::Key_A..Key_P style keys)
	static bool    hasKey(int key);
	static QString textForKey(int key);

	void popup();                       // re-reads the key list, then shows
	void dismiss();
	bool isOpen() const { return isVisible(); }

	// True when the byte sequence is one people press several times in a row
	// (Tab, arrows, paging, Backspace): those must not close the panel.
	static bool keepsOpen(const QByteArray &bytes);

signals:
	void symbolChosen(const QString &text);   // utf8 encoded by the display

	// Raw control bytes from a custom key button.
	void controlKeyChosen(const QString &bytes, bool keepOpen);

protected:
	void paintEvent(QPaintEvent *event);
	bool eventFilter(QObject *watched, QEvent *event);

private:
	void buildCells();
	void rebuildCtrlCells();     // reload CKeyBar::loadItems()
	void relayout();
	void styleCell(QLabel *l, bool ctrl, bool pressed) const;

	// symbol grid
	QList<QLabel *> _cells;      // one label per grid cell, row-major
	QList<QString>  _texts;      // symbol inserted by that cell
	QList<int>      _rowLens;    // cells per row (Term50 row lengths)
	QList<int>      _keyHints;   // source key per cell, 0 when none

	// custom control keys (from CKeyBar)
	QList<CKeyBar::Item> _ctrlItems;
	QList<QLabel *>      _ctrlCells;    // reusable pool of cells
	QList<int>           _ctrlItemIdx;  // cell -> index into _ctrlItems
	QLabel              *_pagePrev;
	QLabel              *_pageNext;
	int                  _ctrlPage;     // 0 based
	int                  _ctrlPages;
	int                  _ctrlCols;

	QLabel *_title;
	QLabel *_hint;

	int _cellHeight;
	int _margin;
};

#endif // SYMBOLPANEL_H
