/*
 * Term50 "main_symmenu" overlay for BBShellBB10.
 *
 * Triggered by the physical SYM key (TerminalDisplay::keyPressEvent), it shows
 * Term50's default symbol grid and inserts the picked glyph into the pty.
 */
#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include "symbolpanel.h"

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

CSymbolPanel::CSymbolPanel(QWidget *parent)
	: QWidget(parent)
	, _cellHeight(72)
	, _margin(6)
{
	setFocusPolicy(Qt::NoFocus);
	setVisible(false);
	buildCells();
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
			cell->setStyleSheet(QString(
			    "QLabel { background-color: rgb(38,42,50);"
			    " border: 1px solid rgb(78,86,102);"
			    " border-radius: 6px; }"));
			cell->installEventFilter(this);
			_cells.append(cell);
			_texts.append(QString::fromUtf8(kSymRows[r][c].text));
			_keyHints.append(kSymRows[r][c].key);
		}
	}
}

// Lays the panel out over the bottom of the terminal: Term50 draws the grid
// full width with each row split evenly, so row 2 (9 cells) gets wider cells.
void CSymbolPanel::relayout()
{
	QWidget *p = parentWidget();
	if (!p || _rowLens.isEmpty())
		return;

	const int rows = _rowLens.size();

	int totalW = p->width() - 12;
	if (totalW > 900) totalW = 900;
	if (totalW < 240) totalW = 240;

	int cellH = totalW / 10;      // square-ish cells, like Term50's rows
	if (cellH > 96) cellH = 96;
	if (cellH < 54) cellH = 54;
	_cellHeight = cellH;

	const int gridW = totalW - 2 * _margin;
	const int gridH = rows * cellH;

	setGeometry((p->width() - totalW) / 2, p->height() - gridH - 2 * _margin - 8,
	            totalW, gridH + 2 * _margin);

	const int symPx  = (_cellHeight * 52) / 100;
	const int hintPx = (_cellHeight * 24) / 100;

	int idx = 0;
	for (int r = 0; r < rows; ++r) {
		const int len = _rowLens[r];
		const int cellW = gridW / len;

		for (int c = 0; c < len; ++c, ++idx) {
			if (idx >= _cells.size())
				break;

			QLabel *cell = _cells[idx];
			cell->setGeometry(_margin + c * cellW, _margin + r * cellH,
			                  cellW - 2, cellH - 2);

			const QString hint = QChar((ushort)_keyHints[idx]).toUpper();
			cell->setText(QString(
			    "<div align=\"center\">"
			    "<span style=\"font-size:%1px; color:rgb(238,238,238);\">%2</span>"
			    "<br/>"
			    "<span style=\"font-size:%3px; color:rgb(0,255,102);\">%4</span>"
			    "</div>")
			    .arg(symPx)
			    .arg(htmlEscape(_texts[idx]))
			    .arg(hintPx)
			    .arg(hint));
		}
	}
}

void CSymbolPanel::popup()
{
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
	painter.setPen(QPen(QColor(0, 255, 102), 2));
	painter.setBrush(QColor(16, 18, 22));
	painter.drawRoundedRect(frame, 8, 8);
}

bool CSymbolPanel::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress ||
	    event->type() == QEvent::MouseButtonRelease) {
		int idx = -1;
		for (int i = 0; i < _cells.size(); ++i) {
			if (_cells[i] == watched) {
				idx = i;
				break;
			}
		}
		if (idx >= 0 && event->type() == QEvent::MouseButtonRelease) {
			const QString text = _texts.value(idx);
			dismiss();
			if (!text.isEmpty())
				emit symbolChosen(text);
		}
		return true;   // never let the tap reach the terminal
	}

	return QWidget::eventFilter(watched, event);
}
