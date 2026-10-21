/*
 * Quick commands + saved SSH hosts for BGShellBB10.
 *
 * Two pages in one overlay:
 *   Commands - a named list of shell one-liners. Tap a row, then either
 *              "Insert" (puts the text on the current command line, so it can
 *              still be edited) or "Run" (text + Enter).
 *   SSH      - host / user / port / key / optional password. "Connect" types
 *              the ssh command line and, when a password was saved, lets
 *              TerminalDisplay answer the password prompt through the pty.
 *
 * Everything is stored in QSettings next to FontSize/ImeOn:
 *   QuickCommands = QStringList "name\tcommand"
 *   SshHosts      = QStringList "name\tuser\thost\tport\tkey\tpassword"
 * The password is only obfuscated (XOR + base64), not encrypted - it lives in
 * the app's private data dir, reachable by root/backup. Keys are better; see
 * README.
 */
#ifndef CMDPANEL_H
#define CMDPANEL_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QToolButton;

class CCommandPanel : public QWidget
{
	Q_OBJECT

public:
	explicit CCommandPanel(QWidget *parent = 0);

	void popup();
	void dismiss();
	bool isOpen() const { return isVisible(); }

signals:
	// run=false -> put the text on the command line only
	void commandChosen(const QString &command, bool run);
	void sshChosen(const QString &user, const QString &host, int port,
	               const QString &identity, const QString &password);

protected:
	void paintEvent(QPaintEvent *event);

private slots:
	void onTabCommands();
	void onTabSsh();
	void onCommandActivated(QListWidgetItem *item);
	void onSshActivated(QListWidgetItem *item);
	void onRun();
	void onInsert();
	void onConnect();
	void onAdd();
	void onDelete();
	void onSave();
	void onCancel();
	void onClose();

private:
	enum Page { PageCommands = 0, PageSsh = 1 };

	void buildUi();
	void setPage(int page);
	void reload();
	void reloadCommands();
	void reloadSsh();
	void showEditor(bool on);
	void hideEditor();
	QListWidget *currentList() const;

	static QStringList commandEntries();
	static QStringList sshEntries();
	static void storeCommandEntries(const QStringList &list);
	static void storeSshEntries(const QStringList &list);
	static QString encodeSecret(const QString &plain);
	static QString decodeSecret(const QString &stored);

	QLabel      *_title;
	QLabel      *_subtitle;
	QToolButton *_tabCommands;
	QToolButton *_tabSsh;
	QStackedWidget *_pages;
	QListWidget *_cmdList;
	QListWidget *_sshList;

	QWidget     *_editor;
	QLineEdit   *_edName;
	QLabel      *_edCommandLabel;
	QLineEdit   *_edCommand;
	QLabel      *_edUserLabel;
	QLineEdit   *_edUser;
	QLabel      *_edHostLabel;
	QLineEdit   *_edHost;
	QLabel      *_edPortLabel;
	QLineEdit   *_edPort;
	QLabel      *_edKeyLabel;
	QLineEdit   *_edKey;
	QLabel      *_edPasswordLabel;
	QLineEdit   *_edPassword;

	QToolButton *_btnSave;
	QToolButton *_btnCancel;
	QToolButton *_btnRun;
	QToolButton *_btnInsert;
	QToolButton *_btnConnect;
	QToolButton *_btnAdd;
	QToolButton *_btnDelete;
	QToolButton *_btnClose;

	int _page;
	int _editIndex;   // row being edited, -1 when adding a new one
};

#endif // CMDPANEL_H
