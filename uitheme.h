/*
 * One look for every surface: system menu, settings, command panel, dialogs.
 *
 * The app talks to a shell, so the chrome imitates a good terminal theme:
 * near-black backdrop, one cool surface tone, thin borders, a single green
 * accent and a muted grey for secondary text. Everything else is spacing.
 *
 * Qt 4.8 / BB10 notes: the sheet below sticks to properties the Qt style
 * sheet engine really supports on this platform (no box-shadow, no
 * selectors that need extra pseudo states). Widgets that paint themselves
 * (TerminalDisplay, ImePanel, SymbolPanel) are deliberately not targeted.
 */
#ifndef UITHEME_H
#define UITHEME_H

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtCore/QString>

class QApplication;
class QMenu;
class QDialog;
class QWidget;
class QVBoxLayout;
class QFrame;

namespace UiTheme {

// palette ---------------------------------------------------------------
extern const QColor Bg;        // page backdrop, also the terminal default bg
extern const QColor Surface;   // cards, menus, dialogs
extern const QColor Surface2;  // inputs, list rows
extern const QColor Border;
extern const QColor Text;
extern const QColor TextDim;
extern const QColor Accent;    // green: primary actions, selection
extern const QColor AccentInk; // readable text on top of Accent
extern const QColor Danger;
extern const QColor Warn;

// helpers ---------------------------------------------------------------
QString hex(const QColor &c);
QFont monoFont(int pixelSize, bool bold = false);
QFont uiFont(int pixelSize, bool bold = false);

// Install the application font and the global sheet.
void apply(QApplication &app);

// Rounded, padded long-press menu (a plain QMenu looks like 2003).
void styleMenu(QMenu *menu);

// A "card": rounded surface + a title line, with a body layout for the
// caller to fill. `subtitle` may be empty.
QFrame *newCard(QWidget *parent, const QString &title,
                const QString &subtitle, QVBoxLayout *&body);

// Small helpers used by the settings window and the command panel.
QWidget *newHeader(QWidget *parent, const QString &title, const QString &subtitle);
QString titleStyle();
QString subtitleStyle();
QString sectionStyle();

} // namespace UiTheme

#endif // UITHEME_H
