/*
 * Term50 "main_symmenu" overlay for BGShellBB10, with the custom key strip
 * folded in. See symbolpanel.h for the layout.
 */
#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include "symbolpanel.h"
#include "uitheme.h"
#include "keybar.h"

namespace {

struct SymEntry {
	int         key;    // Qt::Key_* of the key this symbol is bound to
	const char *text;   // utf8 text to insert
};

// Term50 preferences.c: DEFAULT_SYMMENU_ROW_LENS { 10, 9 } and
// DEFAULT_SYMMENU_ENTRIES, kept verbatim so both apps show the same grid.
const SymEntry kSymRow0[] = {
	{ Qt::Key_Q, "~" }, { Qt::Key_W, "`" }, { Qt::Key_E, "{" }, { Qt::Key_R, "}" },
	{ Qt::Key_T, "[" }, { Qt::Key_Y, "]" }, { Qt::Key_U, "<" }, { Qt::Key_I, ">" },
	{ Qt::Key_O, "^" }, { Qt::Key_P, "%" }, { 0, 0 }
};

const SymEntry kSymRow1[] = {
	{ Qt::Key_A, "=" }, { Qt::Key_S, "-" }, { Qt::Key_D, "*" }, { Qt::Key_F, "/" },
	{ Qt::Key_G, "\\" }, { Qt::Key_H, "|" }, { Qt::Key_J, "&" }, { Qt::Key_K, "'" },
	{ Qt::Key_L, "\"" }, { 0, 0 }
};

const SymEntry *const kSymRows[] = { kSymRow0, kSymRow1, 0 };

// One palette for the whole sheet, derived from UiTheme so the symbol cells,
// the key strip and the candidate strip cannot drift apart again - they used
// to carry three different greens for the same accent.
static QString cellStyle(bool ctrl, bool pressed)
{
	QColor fill = UiTheme::Surface2;
	QColor edge = UiTheme::Border;
	QColor ink  = UiTheme::Text;

	if (ctrl) {
		// A modifier is not a character: warm tint, so the finger learns that
		// tapping it changes the next key rather than typing something.
		fill = UiTheme::blend(UiTheme::Surface2, UiTheme::Warn, 18);
		edge = UiTheme::blend(UiTheme::Border, UiTheme::Warn, 55);
	}
	if (pressed) {
		// Solid accent with dark ink: this is a strike, not a selection.
		fill = UiTheme::Accent;
		edge = UiTheme::Accent;
		ink  = UiTheme::AccentInk;
	}

	return QString("QLabel { background-color: %1; border: 1px solid %2;"
	               " border-radius: 8px; color: %3; }")
	    .arg(UiTheme::hex(fill), UiTheme::hex(edge), UiTheme::hex(ink));
}

// Pager chips: same family as a keycap, quieter ink.
static QString pageStyle()
{
	return QString("QLabel { background-color: %1; border: 1px solid %2;"
	               " border-radius: 8px; color: %3; }")
	    .arg(UiTheme::hex(UiTheme::Surface2),
	         UiTheme::hex(UiTheme::Border),
	         UiTheme::hex(UiTheme::TextDim));
}

QString htmlEscape(const QString &s)
{
	QString out(s);
	out.replace(QChar('&'), QString("&amp;"));
	out.replace(QChar('<'), QString("&lt;"));
	out.replace(QChar('>'), QString("&gt;"));
	return out;
}

} // namespace

bool CSymbolPanel::hasKey(int key)
{
	for (int r = 0; kSymRows[r] != 0; ++r)
		for (const SymEntry *e = kSymRows[r]; e->text != 0; ++e)
			if (e->key == key)
				return true;
	return false;
}

QString CSymbolPanel::textForKey(int key)
{
	for (int r = 0; kSymRows[r] != 0; ++r)
		for (const SymEntry *e = kSymRows[r]; e->text != 0; ++e)
			if (e->key == key)
				return QString::fromUtf8(e->text);
	return QString();
}

bool CSymbolPanel::keepsOpen(const QByteArray &bytes)
{
	if (bytes.isEmpty())
		return false;
	// Tab / Shift+Tab
	if (bytes == QByteArray("\t", 1) || bytes == QByteArray("\x1b[Z", 3))
		return true;
	// Backspace in both of its spellings
	if (bytes == QByteArray("\x7f", 1) || bytes == QByteArray("\x08", 1))
		return true;
	// Arrows, Home/End/Insert/Delete and PgUp/PgDn are CSI sequences that
	// people repeat; anything that looks like one keeps the panel up.
	if (bytes.size() >= 3 && bytes.at(0) == '\x1b' && bytes.at(1) == '[') {
		const char last = bytes.at(bytes.size() - 1);
		if ((last >= 'A' && last <= 'D') || last == 'H' || last == 'F' ||
		    last == '~')
			return true;
	}
	// A lone space or a plain letter should let the panel close: those are
	// usually one-shot inserts, not repeats.
	return false;
}

CSymbolPanel::CSymbolPanel(QWidget *parent)
	: QWidget(parent)
	, _pagePrev(0)
	, _pageNext(0)
	, _ctrlPage(0)
	, _ctrlPages(1)
	, _ctrlCols(6)
	, _title(0)
	, _hint(0)
	, _cellHeight(72)
	, _margin(6)
{
	setFocusPolicy(Qt::NoFocus);
	setVisible(false);
	setAutoFillBackground(false);

	_title = new QLabel(this);
	_title->setFocusPolicy(Qt::NoFocus);
	_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	_title->setText(QString::fromLatin1("SYM"));
	_title->setStyleSheet(QString(
	    "QLabel { background: transparent; color: %1;"
	    " font-weight: bold; letter-spacing: 1px; }")
	    .arg(UiTheme::hex(UiTheme::Accent)));

	_hint = new QLabel(this);
	_hint->setFocusPolicy(Qt::NoFocus);
	_hint->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	_hint->setStyleSheet(QString(
	    "QLabel { background: transparent; color: %1; }")
	    .arg(UiTheme::hex(UiTheme::TextDim)));

	buildCells();
	rebuildCtrlCells();
}

void CSymbolPanel::styleCell(QLabel *l, bool ctrl, bool pressed) const
{
	if (!l)
		return;
	l->setStyleSheet(cellStyle(ctrl, pressed));
}

void CSymbolPanel::buildCells()
{
	_rowLens.clear();

	for (int r = 0; kSymRows[r] != 0; ++r) {
		int n = 0;
		while (kSymRows[r][n].text != 0)
			++n;
		_rowLens.append(n);

		for (int c = 0; c < n; ++c) {
			QLabel *cell = new QLabel(this);
			cell->setAlignment(Qt::AlignCenter);
			cell->setFocusPolicy(Qt::NoFocus);
			styleCell(cell, false, false);
			cell->installEventFilter(this);
			_cells.append(cell);
			_texts.append(QString::fromUtf8(kSymRows[r][c].text));
			_keyHints.append(kSymRows[r][c].key);
		}
	}
}

void CSymbolPanel::rebuildCtrlCells()
{
	_ctrlItems = CKeyBar::loadItems();
	if (_ctrlPage < 0)
		_ctrlPage = 0;
}

// Lays out title + symbol grid + two rows of custom keys over the bottom of
// the terminal. Everything is sized from the panel width, so the Q10's narrow
// screen and the Passport's square one both get full width cells.
void CSymbolPanel::relayout()
{
	QWidget *p = parentWidget();
	if (!p || _rowLens.isEmpty())
		return;

	int totalW = p->width() - 12;
	if (totalW > 900) totalW = 900;
	if (totalW < 240) totalW = 240;

	int cellH = totalW / 10;           // square-ish cells, like Term50's rows
	if (cellH > 96) cellH = 96;
	if (cellH < 54) cellH = 54;
	_cellHeight = cellH;

	const int gridW  = totalW - 2 * _margin;
	const int rows   = _rowLens.size();
	const int titleH = qMax(30, (cellH * 46) / 100);

	int ctrlCellH = (cellH * 62) / 100;
	if (ctrlCellH < 40) ctrlCellH = 40;

	int ctrlCols = gridW / qMax(1, (cellH * 13) / 10);
	if (ctrlCols < 3) ctrlCols = 3;
	if (ctrlCols > 8) ctrlCols = 8;
	_ctrlCols = ctrlCols;

	// Two rows of keys; when the list does not fit, the last two slots turn
	// into the pager so a large key set is still reachable.
	const int slotCount = ctrlCols * 2;
	const int n     = _ctrlItems.size();
	int cap = slotCount > 2 ? slotCount - 2 : slotCount;
	int pages = cap > 0 ? (n + cap - 1) / cap : 1;
	if (pages < 1) pages = 1;
	if (pages == 1)
		cap = slotCount;
	_ctrlPages = pages;
	if (_ctrlPage >= pages) _ctrlPage = pages - 1;
	if (_ctrlPage < 0)      _ctrlPage = 0;

	const int gap      = 8;
	const int ctrlAreaH = ctrlCellH * 2;
	const int panelH = _margin + titleH + rows * cellH + gap + ctrlAreaH + _margin;

	int y = p->height() - panelH - 8;
	if (y < 0) y = 0;
	setGeometry((p->width() - totalW) / 2, y, totalW, panelH);

	// --- title line ----------------------------------------------------
	_title->setGeometry(_margin + 2, _margin, gridW / 2, titleH);
	_hint->setGeometry(_margin + gridW / 2, _margin, gridW / 2 - 4, titleH);
	{
		QFont f = _title->font();
		f.setPixelSize(qMax(15, (titleH * 52) / 100));
		f.setBold(true);
		_title->setFont(f);
	}
	{
		QFont f = _hint->font();
		f.setPixelSize(qMax(11, (titleH * 36) / 100));
		_hint->setFont(f);
		if (_ctrlPages > 1)
			_hint->setText(QString::fromUtf8("控制键 %1/%2  ·  SYM 关闭")
			                   .arg(_ctrlPage + 1).arg(_ctrlPages));
		else
			_hint->setText(QString::fromUtf8("SYM / Esc 关闭"));
	}

	// --- symbol grid ---------------------------------------------------
	const int symPx  = (cellH * 52) / 100;
	const int hintPx = (cellH * 24) / 100;

	int idx = 0;
	for (int r = 0; r < rows; ++r) {
		const int len   = _rowLens[r];
		const int cellW = gridW / len;

		for (int c = 0; c < len; ++c, ++idx) {
			if (idx >= _cells.size())
				break;

			QLabel *cell = _cells[idx];
			cell->setGeometry(_margin + c * cellW, _margin + titleH + r * cellH,
			                  cellW - 2, cellH - 2);

			const QString hint = QChar((ushort)_keyHints[idx]).toUpper();
			// The glyph is the cell; the key hint is a footnote under it, so it
			// gets the secondary ink instead of a second green.
			cell->setText(QString(
			    "<div align=\"center\">"
			    "<span style=\"font-size:%1px; color:%2;\">%3</span>"
			    "<br/>"
			    "<span style=\"font-size:%4px; color:%5;\">%6</span>"
			    "</div>")
			    .arg(symPx)
			    .arg(UiTheme::hex(UiTheme::Text))
			    .arg(htmlEscape(_texts[idx]))
			    .arg(hintPx)
			    .arg(UiTheme::hex(UiTheme::TextDim))
			    .arg(hint));
		}
	}

	// --- custom key area -----------------------------------------------
	const int ctrlTop   = _margin + titleH + rows * cellH + gap;
	const int ctrlCellW = gridW / ctrlCols;
	const int ctrlPx    = qMax(13, (ctrlCellH * 44) / 100);

	const int first   = _ctrlPage * cap;
	int visible = n - first;
	if (visible > cap) visible = cap;
	if (visible < 0)   visible = 0;

	while (_ctrlCells.size() < visible) {
		QLabel *b = new QLabel(this);
		b->setAlignment(Qt::AlignCenter);
		b->setFocusPolicy(Qt::NoFocus);
		styleCell(b, true, false);
		b->installEventFilter(this);
		_ctrlCells.append(b);
	}
	_ctrlItemIdx.clear();
	for (int i = 0; i < _ctrlCells.size(); ++i) {
		_ctrlCells[i]->setVisible(i < visible);
		_ctrlItemIdx.append(i < visible ? first + i : -1);
	}

	for (int i = 0; i < visible; ++i) {
		const int row = i / ctrlCols;
		const int col = i % ctrlCols;
		QLabel *b = _ctrlCells[i];
		b->setGeometry(_margin + col * ctrlCellW, ctrlTop + row * ctrlCellH,
		               ctrlCellW - 2, ctrlCellH - 2);

		const CKeyBar::Item &it = _ctrlItems.at(first + i);
		b->setText(QString(
		    "<div align=\"center\">"
		    "<span style=\"font-size:%1px; color:rgb(190,255,210);\">%2</span>"
		    "</div>")
		    .arg(ctrlPx)
		    .arg(htmlEscape(it.label)));
	}

	// --- pager ---------------------------------------------------------
	if (!_pagePrev) {
		_pagePrev = new QLabel(this);
		_pagePrev->setAlignment(Qt::AlignCenter);
		_pagePrev->setFocusPolicy(Qt::NoFocus);
		_pagePrev->setStyleSheet(pageStyle());
		_pagePrev->installEventFilter(this);
	}
	if (!_pageNext) {
		_pageNext = new QLabel(this);
		_pageNext->setAlignment(Qt::AlignCenter);
		_pageNext->setFocusPolicy(Qt::NoFocus);
		_pageNext->setStyleSheet(pageStyle());
		_pageNext->installEventFilter(this);
	}

	if (_ctrlPages > 1) {
		const int ps = ctrlCellH;
		{
			const int row = (slotCount - 2) / ctrlCols;
			const int col = (slotCount - 2) % ctrlCols;
			_pagePrev->setGeometry(_margin + col * ctrlCellW,
			                       ctrlTop + row * ctrlCellH,
			                       ctrlCellW - 2, ctrlCellH - 2);
			_pagePrev->setText(QString::fromUtf8(
			    "<span style=\"font-size:%1px; color:rgb(220,220,220);\">‹</span>")
			    .arg((ps * 60) / 100));
			_pagePrev->show();
		}
		{
			const int row = (slotCount - 1) / ctrlCols;
			const int col = (slotCount - 1) % ctrlCols;
			_pageNext->setGeometry(_margin + col * ctrlCellW,
			                       ctrlTop + row * ctrlCellH,
			                       ctrlCellW - 2, ctrlCellH - 2);
			_pageNext->setText(QString::fromUtf8(
			    "<span style=\"font-size:%1px; color:rgb(220,220,220);\">›</span>")
			    .arg((ps * 60) / 100));
			_pageNext->show();
		}
	} else {
		_pagePrev->hide();
		_pageNext->hide();
	}
}

void CSymbolPanel::popup()
{
	rebuildCtrlCells();
	relayout();
	show();
	raise();
}

void CSymbolPanel::dismiss()
{
	hide();
	if (parentWidget())
		parentWidget()->setFocus();
}

void CSymbolPanel::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	QRectF frame(rect().adjusted(0, 0, -1, -1));
	// A sheet, not a lightbox: the 2px neon frame was the loudest thing on
	// screen and it framed a panel that is only a secondary input.
	painter.setPen(QPen(UiTheme::Border, 1));
	painter.setBrush(UiTheme::Surface);
	painter.drawRoundedRect(frame, 14, 14);

	// hairline under the title
	const int titleH = qMax(30, (_cellHeight * 46) / 100);
	const int ly = _margin + titleH - 4;
	painter.setPen(QPen(UiTheme::Border, 1));
	painter.drawLine(_margin, ly, width() - _margin, ly);
}

bool CSymbolPanel::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() != QEvent::MouseButtonPress &&
	    event->type() != QEvent::MouseButtonRelease)
		return QWidget::eventFilter(watched, event);

	QLabel *l = qobject_cast<QLabel *>(watched);
	const bool press = (event->type() == QEvent::MouseButtonPress);

	int symIdx = -1;
	for (int i = 0; i < _cells.size(); ++i)
		if (_cells[i] == watched) { symIdx = i; break; }

	int ctrlIdx = -1;
	for (int i = 0; i < _ctrlCells.size(); ++i)
		if (_ctrlCells[i] == watched) { ctrlIdx = i; break; }

	const bool pager = (l && (l == _pagePrev || l == _pageNext));

	if (press) {
		if (symIdx >= 0)
			styleCell(_cells[symIdx], false, true);
		else if (ctrlIdx >= 0)
			styleCell(_ctrlCells[ctrlIdx], true, true);
		else if (pager)
			l->setStyleSheet(cellStyle(false, true));
		return true;   // never let the tap reach the terminal
	}

	if (symIdx >= 0) {
		styleCell(_cells[symIdx], false, false);
		const QString text = _texts.value(symIdx);
		dismiss();
		if (!text.isEmpty())
			emit symbolChosen(text);
	} else if (ctrlIdx >= 0) {
		styleCell(_ctrlCells[ctrlIdx], true, false);
		const int itemIdx = _ctrlItemIdx.value(ctrlIdx, -1);
		if (itemIdx >= 0 && itemIdx < _ctrlItems.size()) {
			const CKeyBar::Item it = _ctrlItems.at(itemIdx);
			const bool keep = keepsOpen(it.payload);
			if (!keep)
				dismiss();
			if (!it.payload.isEmpty())
				emit controlKeyChosen(QString::fromUtf8(it.payload), keep);
		}
	} else if (pager) {
		l->setStyleSheet(pageStyle());
		if (_ctrlPages > 1) {
			if (l == _pageNext)
				_ctrlPage = (_ctrlPage + 1) % _ctrlPages;
			else
				_ctrlPage = (_ctrlPage - 1 + _ctrlPages) % _ctrlPages;
			relayout();
		}
	}

	return true;
}
