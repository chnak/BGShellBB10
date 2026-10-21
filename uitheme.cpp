#include "uitheme.h"

#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFontDatabase>

// The bundled CJK face; empty when the asset is missing (see main.cpp).
extern QString gTerminalFontFamily;

namespace UiTheme {

const QColor Bg        (0x0b, 0x0e, 0x11);
const QColor Surface   (0x14, 0x1a, 0x1f);
const QColor Surface2  (0x1b, 0x22, 0x29);
const QColor Border    (0x2b, 0x35, 0x40);
const QColor Text      (0xe8, 0xed, 0xf2);
const QColor TextDim   (0x8d, 0x9a, 0xa7);
const QColor Accent    (0x00, 0xe0, 0x5a);
const QColor AccentInk (0x04, 0x14, 0x0a);
const QColor Danger    (0xff, 0x5f, 0x56);
const QColor Warn      (0xe3, 0xb3, 0x41);

QString hex(const QColor &c)
{
	return c.name();
}

QFont monoFont(int pixelSize, bool bold)
{
	QFont f(gTerminalFontFamily.isEmpty() ? QString("Courier New")
	                                      : gTerminalFontFamily);
	f.setPixelSize(pixelSize);
	f.setBold(bold);
	f.setStyleStrategy(QFont::PreferAntialias);
	return f;
}

QFont uiFont(int pixelSize, bool bold)
{
	QFont f = QApplication::font();
	f.setPixelSize(pixelSize);
	f.setBold(bold);
	return f;
}

QString titleStyle()
{
	return QString("QLabel { color: %1; font-size: 34px; font-weight: bold; }")
	    .arg(hex(Text));
}

QString subtitleStyle()
{
	return QString("QLabel { color: %1; font-size: 21px; }").arg(hex(TextDim));
}

QString sectionStyle()
{
	return QString("QLabel { color: %1; font-size: 19px; font-weight: bold;"
	               " letter-spacing: 1px; }")
	    .arg(hex(TextDim));
}

QWidget *newHeader(QWidget *parent, const QString &title, const QString &subtitle)
{
	QWidget *w = new QWidget(parent);
	QVBoxLayout *l = new QVBoxLayout(w);
	l->setContentsMargins(4, 0, 4, 0);
	l->setSpacing(2);

	QLabel *t = new QLabel(title, w);
	t->setStyleSheet(titleStyle());
	l->addWidget(t);

	if (!subtitle.isEmpty()) {
		QLabel *s = new QLabel(subtitle, w);
		s->setStyleSheet(subtitleStyle());
		s->setWordWrap(true);
		l->addWidget(s);
	}
	return w;
}

QFrame *newCard(QWidget *parent, const QString &title,
                const QString &subtitle, QVBoxLayout *&body)
{
	QFrame *card = new QFrame(parent);
	card->setObjectName(QString("card"));
	card->setStyleSheet(QString(
	    "QFrame#card { background-color: %1; border: 1px solid %2;"
	    " border-radius: 18px; }")
	    .arg(hex(Surface), hex(Border)));

	QVBoxLayout *outer = new QVBoxLayout(card);
	outer->setContentsMargins(18, 16, 18, 16);
	outer->setSpacing(8);

	if (!title.isEmpty()) {
		QLabel *t = new QLabel(title, card);
		t->setStyleSheet(sectionStyle());
		outer->addWidget(t);
	}
	if (!subtitle.isEmpty()) {
		QLabel *s = new QLabel(subtitle, card);
		s->setStyleSheet(subtitleStyle());
		s->setWordWrap(true);
		outer->addWidget(s);
	}

	QWidget *holder = new QWidget(card);
	body = new QVBoxLayout(holder);
	body->setContentsMargins(0, 6, 0, 0);
	body->setSpacing(10);
	outer->addWidget(holder);

	return card;
}

void styleMenu(QMenu *menu)
{
	if (!menu)
		return;
	menu->setStyleSheet(QString(
	    "QMenu { background-color: %1; border: 1px solid %2;"
	    " border-radius: 16px; padding: 10px; }"
	    "QMenu::item { padding: 18px 34px 18px 24px; border-radius: 10px;"
	    " color: %3; font-size: 26px; }"
	    "QMenu::item:selected { background-color: #16301f; color: #ffffff; }"
	    "QMenu::separator { height: 1px; background-color: %2;"
	    " margin: 8px 14px; }")
	    .arg(hex(Surface), hex(Border), hex(Text)));
}

void apply(QApplication &app)
{
	// One family everywhere keeps the UI and the terminal visually related.
	QFont base = app.font();
	base.setPixelSize(24);
	app.setFont(base);

	QString s;

	// ---- buttons ------------------------------------------------------
	s += QString(
	    "QToolButton, QPushButton {"
	    "  background-color: %1;"
	    "  color: %2;"
	    "  border: 1px solid %3;"
	    "  border-radius: 14px;"
	    "  padding: 12px 18px;"
	    "  font-size: 25px;"
	    "}"
	    "QToolButton:pressed, QPushButton:pressed {"
	    "  background-color: #24303a; border-color: %4;"
	    "}"
	    "QToolButton:disabled, QPushButton:disabled {"
	    "  color: #5a6673; border-color: #202830;"
	    "}"
	    // primary action: green plate, dark ink
	    "QToolButton#primary, QPushButton#primary {"
	    "  background-color: %4; color: %5; border: 1px solid %4;"
	    "  font-weight: bold;"
	    "}"
	    "QToolButton#primary:pressed, QPushButton#primary:pressed {"
	    "  background-color: #00c24e; border-color: #00c24e;"
	    "}"
	    // quiet secondary
	    "QToolButton#secondary, QPushButton#secondary {"
	    "  background-color: transparent; border-color: %3; color: %2;"
	    "}"
	    "QToolButton#secondary:pressed, QPushButton#secondary:pressed {"
	    "  background-color: #1c262e;"
	    "}"
	    // destructive
	    "QToolButton#danger, QPushButton#danger {"
	    "  background-color: transparent; border-color: #3a2226;"
	    "  color: #ff8a80;"
	    "}"
	    "QToolButton#danger:pressed, QPushButton#danger:pressed {"
	    "  background-color: #2a1416;"
	    "}"
	    // square icon-ish button (close X)
	    "QToolButton#icon, QPushButton#icon {"
	    "  background-color: transparent; border: 1px solid %3;"
	    "  color: %6; padding: 0px; font-size: 30px;"
	    "}"
	    "QToolButton#icon:pressed, QPushButton#icon:pressed {"
	    "  background-color: #1c262e;"
	    "}"
	    // segmented tab
	    "QToolButton#segment {"
	    "  background-color: transparent; border: 1px solid %3;"
	    "  color: %6; font-size: 24px; padding: 10px 12px;"
	    "}"
	    "QToolButton#segment:checked {"
	    "  background-color: #11301f; border: 1px solid %4;"
	    "  color: %4; font-size: 24px; padding: 10px 12px; font-weight: bold;"
	    "}")
	    .arg(hex(Surface2), hex(Text), hex(Border), hex(Accent),
	         hex(AccentInk), hex(TextDim));

	// ---- text inputs --------------------------------------------------
	s += QString(
	    "QLineEdit {"
	    "  background-color: %1; color: %2; border: 1px solid %3;"
	    "  border-radius: 12px; padding: 10px 14px; font-size: 24px;"
	    "  selection-background-color: #1f4d33; selection-color: #ffffff;"
	    "}"
	    "QLineEdit:focus { border: 1px solid %4; }"
	    "QComboBox {"
	    "  background-color: %1; color: %2; border: 1px solid %3;"
	    "  border-radius: 12px; padding: 8px 14px; min-height: 46px;"
	    "  font-size: 24px;"
	    "}"
	    "QComboBox::drop-down { width: 40px; border: none; }"
	    "QComboBox QAbstractItemView {"
	    "  background-color: %5; color: %2; border: 1px solid %3;"
	    "  selection-background-color: #1f4d33; selection-color: #ffffff;"
	    "  outline: none; padding: 4px;"
	    "}"
	    "QCheckBox { color: %2; font-size: 24px; spacing: 14px; }"
	    "QCheckBox::indicator {"
	    "  width: 40px; height: 40px; border-radius: 10px;"
	    "  border: 1px solid %3; background-color: %1;"
	    "}"
	    "QCheckBox::indicator:checked {"
	    "  background-color: %4; border: 1px solid %4;"
	    "}")
	    .arg(hex(Surface2), hex(Text), hex(Border), hex(Accent), hex(Surface));

	// ---- lists / scrollers --------------------------------------------
	s += QString(
	    "QListWidget { background-color: transparent; border: none;"
	    "  outline: none; font-size: 26px; }"
	    "QListWidget::item { border: none; }"
	    "QScrollArea { border: none; background: transparent; }"
	    "QScrollBar:vertical { background: transparent; width: 12px;"
	    "  margin: 2px 0 2px 0; }"
	    "QScrollBar::handle:vertical { background-color: #33404b;"
	    "  border-radius: 6px; min-height: 60px; }"
	    "QScrollBar::handle:vertical:pressed { background-color: %1; }"
	    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
	    "  height: 0px; }"
	    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
	    "  background: transparent; }")
	    .arg(hex(Accent));

	// ---- menus, dialogs, misc -----------------------------------------
	s += QString(
	    "QMenu { background-color: %1; border: 1px solid %2;"
	    "  border-radius: 16px; padding: 10px; }"
	    "QMenu::item { padding: 18px 34px 18px 24px; border-radius: 10px;"
	    "  color: %3; font-size: 26px; }"
	    "QMenu::item:selected { background-color: #16301f; color: #ffffff; }"
	    "QMenu::separator { height: 1px; background-color: %2;"
	    "  margin: 8px 14px; }"
	    "QDialog, QMessageBox { background-color: %1; }"
	    "QMessageBox QLabel { color: %3; font-size: 24px; }"
	    "QGroupBox { border: 1px solid %2; border-radius: 16px;"
	    "  margin-top: 16px; padding: 16px; color: %4; }"
	    "QGroupBox::title { subcontrol-origin: margin; left: 18px;"
	    "  padding: 0 8px; font-size: 19px; }"
	    "QPlainTextEdit, QTextEdit {"
	    "  background-color: #0f1418; color: #d7e0ea;"
	    "  border: 1px solid %2; border-radius: 14px; padding: 10px;"
	    "  selection-background-color: #1f4d33;"
	    "}"
	    "QToolTip { background-color: %1; color: %3;"
	    "  border: 1px solid %2; padding: 6px; }")
	    .arg(hex(Surface), hex(Border), hex(Text), hex(TextDim));

	app.setStyleSheet(s);
}

} // namespace UiTheme
