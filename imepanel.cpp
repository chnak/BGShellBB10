/*
 * The IME candidate strip - see imepanel.h.
 */
#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include "imepanel.h"

namespace {
const int kStripHeight = 78;
const int kMargin      = 8;
const int kChipWidth   = 46;
const int kGap         = 10;
}

CImePanel::CImePanel(QWidget *parent)
	: QWidget(parent)
	, _page(0)
	, _pages(0)
	, _highlight(0)
{
	setFocusPolicy(Qt::NoFocus);
	setVisible(false);

	_glyphFont.setPixelSize(30);
	_smallFont.setPixelSize(17);
}

void CImePanel::setCjkFontFamily(const QString &family)
{
	if (family.isEmpty())
		return;
	_glyphFont.setFamily(family);
	_smallFont.setFamily(family);
	relayout();
}

int CImePanel::heightForStrip() const
{
	return kStripHeight;
}

void CImePanel::setState(const QString &buffer, const QStringList &candidates,
                         int pageIndex, int pageCount)
{
	_buffer     = buffer;
	_candidates = candidates;
	_page       = pageIndex;
	_pages      = pageCount;
	_highlight  = 0;        // a new page always starts at its first candidate
	relayout();
	update();
}

void CImePanel::relayout()
{
	_candidateRects.clear();
	_prevRect = QRect();
	_nextRect = QRect();

	const QFontMetrics small(_smallFont);
	const QFontMetrics glyph(_glyphFont);
	const int top = (kStripHeight - 46) / 2;

	int x = kMargin + kChipWidth + kGap;
	// pinyin that has been typed so far
	x += small.width(_buffer) + kGap * 2;

	// reserve room for the "3/8  < >" block when there is more than one page
	const int reserved = _pages > 1 ? 100 : kMargin;
	const int right = width() - reserved;

	for (int i = 0; i < _candidates.size(); ++i) {
		const QString digit = QString::number(i + 1);
		const int w = small.width(digit) + 3 + glyph.width(_candidates.at(i)) + 12;
		if (x + w > right && !_candidateRects.isEmpty())
			break;          // the rest of the page stays reachable by number key
		_candidateRects.append(QRect(x, top, w, 46));
		x += w;
	}

	// the highlight can only ever point at something that is drawn
	if (_highlight >= _candidateRects.size())
		_highlight = 0;

	if (_pages > 1) {
		const int w = small.width(QString("%1/%2").arg(_page + 1).arg(_pages));
		_nextRect = QRect(kMargin + width() - 30, top, 26, 46);
		_prevRect = _nextRect.translated(-(30 + w + 6), 0);
	}
}

void CImePanel::popup()
{
	if (!parentWidget())
		return;
	const int w = parentWidget()->width();
	setGeometry(0, parentWidget()->height() - kStripHeight, w, kStripHeight);
	relayout();
	show();
	raise();
}

void CImePanel::dismiss()
{
	hide();
}

void CImePanel::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	const QRect r = rect();

	p.fillRect(r, QColor(0, 0, 0, 228));
	p.setPen(QColor(0x00, 0xff, 0x66));
	p.drawLine(r.left(), r.top(), r.right(), r.top());

	const int top = (kStripHeight - 46) / 2;

	// --- the 中 / EN chip ---------------------------------------------
	{
		QPen pen(QColor(0x00, 0xff, 0x66));
		pen.setWidth(1);
		p.setPen(pen);
		const QRect chip(kMargin, top, kChipWidth, 46);
		p.drawRect(chip.adjusted(0, 0, -1, -1));

		p.setFont(_glyphFont);
		p.setPen(QColor(0x00, 0xff, 0x66));
		p.drawText(chip, Qt::AlignCenter, QString::fromUtf8("中"));
	}

	const QFontMetrics small(_smallFont);

	// --- the pinyin typed so far --------------------------------------
	int x = kMargin + kChipWidth + kGap;
	{
		p.setFont(_smallFont);
		p.setPen(QColor(0x00, 0xff, 0x66));
		const QRect buf(x, top, small.width(_buffer) + 4, 46);
		p.drawText(buf, Qt::AlignVCenter | Qt::AlignLeft, _buffer);
		// caret
		p.drawLine(buf.right() + 2, top + 10, buf.right() + 2, top + 36);
		x += small.width(_buffer) + kGap * 2;
	}

	// --- candidates ----------------------------------------------------
	for (int i = 0; i < _candidates.size() && i < _candidateRects.size(); ++i) {
		const QRect cell = _candidateRects.at(i);

		// the one the trackpad / arrow keys are sitting on
		if (i == _highlight) {
			p.fillRect(cell.adjusted(0, 0, -1, -1), QColor(0x00, 0x50, 0x20));
			p.setPen(QColor(0x00, 0xff, 0x66));
			p.drawRect(cell.adjusted(0, 0, -1, -1));
		}

		p.setFont(_smallFont);
		p.setPen(QColor(0x00, 0xc0, 0x50));
		p.drawText(QRect(cell.left() + 2, cell.top(), cell.width(), 18),
		           Qt::AlignLeft | Qt::AlignVCenter, QString::number(i + 1));

		p.setFont(_glyphFont);
		p.setPen(QColor(0xe8, 0xe8, 0xe8));
		p.drawText(QRect(cell.left() + small.width(QString::number(i + 1)) + 4,
		                 cell.top(), cell.width(), 46),
		           Qt::AlignLeft | Qt::AlignVCenter, _candidates.at(i));
	}
	if (_candidates.isEmpty() && !_buffer.isEmpty()) {
		p.setFont(_smallFont);
		p.setPen(QColor(0x90, 0x90, 0x90));
		p.drawText(QRect(x, top, width() - x - 40, 46),
		           Qt::AlignVCenter | Qt::AlignLeft,
		           QString::fromUtf8("没有这个拼音 / no candidate - Esc drops it"));
	}

	// --- paging --------------------------------------------------------
	if (_pages > 1) {
		p.setFont(_smallFont);
		p.setPen(QColor(0x00, 0xff, 0x66));
		p.drawText(_prevRect, Qt::AlignCenter, QString("<"));
		p.drawText(_nextRect, Qt::AlignCenter, QString(">"));

		const int w = small.width(QString("%1/%2").arg(_page + 1).arg(_pages));
		p.setPen(QColor(0xb0, 0xb0, 0xb0));
		p.drawText(QRect(_prevRect.right() + 6, top, w + 4, 46),
		           Qt::AlignCenter, QString("%1/%2").arg(_page + 1).arg(_pages));
	}
}

void CImePanel::mousePressEvent(QMouseEvent *event)
{
	const QPoint pos = event->pos();

	for (int i = 0; i < _candidateRects.size(); ++i) {
		if (_candidateRects.at(i).contains(pos)) {
			emit candidateChosen(i);   // engine commits via its 1..8 keys
			event->accept();
			return;
		}
	}
	if (_pages > 1 && _prevRect.contains(pos)) {
		emit pageRequested(-1);
		event->accept();
		return;
	}
	if (_pages > 1 && _nextRect.contains(pos)) {
		emit pageRequested(+1);
		event->accept();
		return;
	}
	event->ignore();
}

void CImePanel::moveHighlight(int delta)
{
	const int n = _candidateRects.size();
	if (n == 0 || delta == 0)
		return;

	// the trackpad streams one event per pixel, so pace the steps the way a
	// held arrow key repeats instead of flying over the page
	const QTime now = QTime::currentTime();
	if (_lastStep.isValid() && _lastStep.msecsTo(now) < 80)
		return;
	_lastStep = now;

	_highlight = (_highlight + delta) % n;
	if (_highlight < 0)
		_highlight += n;
	update();
}

void CImePanel::commitHighlighted()
{
	if (_highlight < 0 || _highlight >= _candidateRects.size())
		return;
	emit candidateChosen(_highlight);   // engine commits via its 1..8 keys
}
