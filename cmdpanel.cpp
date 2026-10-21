/*
 * Quick commands / saved SSH hosts - see cmdpanel.h.
 */
#include <QtCore/QByteArray>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QScrollArea>
#include <QtGui/QStackedWidget>
#include <QtGui/QStyledItemDelegate>
#include <QtGui/QToolButton>
#include <QtGui/QFontMetrics>

#include "cmdpanel.h"
#include "uitheme.h"

namespace {

const char *kCommandsKey = "QuickCommands";
const char *kSshKey      = "SshHosts";

const int kMargin    = 22;
const int kRowHeight = 104;

// Row data layout: DisplayRole = name, UserRole = index, UserRole+1 = second
// line (the command itself, or user@host:port).
const int kDetailRole = Qt::UserRole + 1;

QToolButton *makeButton(QWidget *parent, const QString &text,
                        const char *kind = 0, int minHeight = 70)
{
	QToolButton *b = new QToolButton(parent);
	b->setText(text);
	if (kind)
		b->setObjectName(QString::fromLatin1(kind));
	b->setMinimumHeight(minHeight);
	b->setFocusPolicy(Qt::NoFocus);
	b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	return b;
}

// Two-line rows: a name in bright mono on top, the payload in dim mono below.
// The stock list item draws one grey line; this is the whole difference
// between "list of strings" and "list of things".
class RowDelegate : public QStyledItemDelegate
{
public:
	explicit RowDelegate(QObject *parent = 0) : QStyledItemDelegate(parent) {}

	QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
	{
		return QSize(160, kRowHeight);
	}

	void paint(QPainter *p, const QStyleOptionViewItem &opt,
	           const QModelIndex &idx) const
	{
		p->save();
		p->setRenderHint(QPainter::Antialiasing, true);

		const bool selected = (opt.state & QStyle::State_Selected) != 0;
		const bool enabled  = idx.data(Qt::UserRole).toInt() >= 0;
		QRect r = opt.rect.adjusted(0, 4, 0, -4);

		if (selected) {
			p->setPen(Qt::NoPen);
			p->setBrush(QColor(0x16, 0x30, 0x1f));
			p->drawRoundedRect(r, 14, 14);
			p->setBrush(UiTheme::Accent);
			p->drawRoundedRect(QRect(r.left() + 10, r.top() + 22, 4,
			                         r.height() - 44), 2, 2);
		} else {
			p->setPen(QPen(QColor(0x1d, 0x25, 0x2c), 1));
			p->drawLine(r.left() + 16, r.bottom(), r.right() - 16, r.bottom());
		}

		const int left  = r.left() + 28;
		const int limit = r.right() - 18 - left;
		if (limit <= 20) {
			p->restore();
			return;
		}

		QFont titleFont = UiTheme::monoFont(26);
		p->setFont(titleFont);
		p->setPen(selected ? QColor(0xff, 0xff, 0xff)
		                   : (enabled ? UiTheme::Text : UiTheme::TextDim));
		const QFontMetrics tfm(titleFont);
		p->drawText(QRect(left, r.top() + 12, limit, 34),
		            Qt::AlignLeft | Qt::AlignVCenter,
		            tfm.elidedText(idx.data(Qt::DisplayRole).toString(),
		                           Qt::ElideRight, limit));

		const QString detail = idx.data(kDetailRole).toString();
		if (!detail.isEmpty()) {
			QFont detailFont = UiTheme::monoFont(21);
			p->setFont(detailFont);
			p->setPen(selected ? QColor(0x8f, 0xe6, 0xa8) : UiTheme::TextDim);
			const QFontMetrics dfm(detailFont);
			p->drawText(QRect(left, r.top() + 48, limit, 30),
			            Qt::AlignLeft | Qt::AlignVCenter,
			            dfm.elidedText(detail, Qt::ElideRight, limit));
		}
		p->restore();
	}
};

// [ label ] over [ line edit ] - full width fields are easier to hit than a
// two column form when every tap is a thumb.
QLineEdit *addField(QVBoxLayout *form, QWidget *parent, const QString &label,
                    const QString &hint, QLabel **labelOut)
{
	QLabel *l = new QLabel(label, parent);
	l->setStyleSheet(QString("QLabel { color: %1; font-size: 20px; }")
	                     .arg(UiTheme::hex(UiTheme::TextDim)));
	form->addWidget(l);

	QLineEdit *e = new QLineEdit(parent);
	e->setPlaceholderText(hint);
	e->setMinimumHeight(64);
	form->addWidget(e);

	if (labelOut)
		*labelOut = l;
	return e;
}

} // namespace

CCommandPanel::CCommandPanel(QWidget *parent)
	: QWidget(parent)
	, _page(PageCommands)
	, _editIndex(-1)
{
	setFocusPolicy(Qt::StrongFocus);
	setVisible(false);
	buildUi();
}

void CCommandPanel::buildUi()
{
	QVBoxLayout *root = new QVBoxLayout(this);
	root->setContentsMargins(kMargin, 18, kMargin, 20);
	root->setSpacing(14);

	// ---- header ------------------------------------------------------
	QHBoxLayout *head = new QHBoxLayout;
	head->setSpacing(12);

	QVBoxLayout *headText = new QVBoxLayout;
	headText->setContentsMargins(0, 0, 0, 0);
	headText->setSpacing(3);

	_title = new QLabel(this);
	_title->setStyleSheet(UiTheme::titleStyle());
	headText->addWidget(_title);

	_subtitle = new QLabel(this);
	_subtitle->setStyleSheet(UiTheme::subtitleStyle());
	_subtitle->setWordWrap(true);
	headText->addWidget(_subtitle);

	head->addLayout(headText, 1);

	_btnClose = makeButton(this, QString::fromUtf8("✕"), "icon", 58);
	_btnClose->setFixedWidth(58);
	head->addWidget(_btnClose, 0, Qt::AlignTop);
	root->addLayout(head);

	// ---- segmented tabs ----------------------------------------------
	QHBoxLayout *tabs = new QHBoxLayout;
	tabs->setSpacing(10);
	_tabCommands = makeButton(this, QString::fromUtf8("命令"), "segment", 62);
	_tabSsh      = makeButton(this, QString::fromUtf8("SSH 主机"), "segment", 62);
	_tabCommands->setCheckable(true);
	_tabSsh->setCheckable(true);
	tabs->addWidget(_tabCommands, 1);
	tabs->addWidget(_tabSsh, 1);
	root->addLayout(tabs);

	// ---- lists --------------------------------------------------------
	_pages = new QStackedWidget(this);

	_cmdList = new QListWidget(_pages);
	_cmdList->setItemDelegate(new RowDelegate(_cmdList));
	_cmdList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	_cmdList->setSelectionMode(QAbstractItemView::SingleSelection);
	_cmdList->setFocusPolicy(Qt::NoFocus);
	_cmdList->setFrameShape(QFrame::NoFrame);
	_pages->addWidget(_cmdList);

	_sshList = new QListWidget(_pages);
	_sshList->setItemDelegate(new RowDelegate(_sshList));
	_sshList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	_sshList->setSelectionMode(QAbstractItemView::SingleSelection);
	_sshList->setFocusPolicy(Qt::NoFocus);
	_sshList->setFrameShape(QFrame::NoFrame);
	_pages->addWidget(_sshList);

	// ---- editor -------------------------------------------------------
	QScrollArea *scroll = new QScrollArea(_pages);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->viewport()->setAutoFillBackground(false);

	_editor = new QWidget(scroll);
	_editor->setAutoFillBackground(false);
	QVBoxLayout *form = new QVBoxLayout(_editor);
	form->setContentsMargins(2, 2, 10, 2);
	form->setSpacing(10);

	_edName = addField(form, _editor, QString::fromUtf8("名称"),
	                   QString::fromUtf8("列表里显示的名字"), 0);

	_edCommand = addField(form, _editor, QString::fromUtf8("命令"),
	                      QString::fromUtf8("例如 ls -l /accounts/1000/shared"),
	                      &_edCommandLabel);

	_edUser = addField(form, _editor, QString::fromUtf8("用户"),
	                   QString::fromUtf8("root"), &_edUserLabel);

	_edHost = addField(form, _editor, QString::fromUtf8("主机"),
	                   QString::fromUtf8("192.168.1.10 或 host.domain"),
	                   &_edHostLabel);

	_edPort = addField(form, _editor, QString::fromUtf8("端口"),
	                   QString::fromUtf8("22"), &_edPortLabel);

	_edKey = addField(form, _editor, QString::fromUtf8("私钥（可选）"),
	                  QString::fromUtf8("data/.ssh/id_rsa"), &_edKeyLabel);

	_edPassword = addField(form, _editor, QString::fromUtf8("密码（可选）"),
	                       QString::fromUtf8("留空则用密钥 / 手动输入"),
	                       &_edPasswordLabel);
	_edPassword->setEchoMode(QLineEdit::Password);

	QHBoxLayout *editButtons = new QHBoxLayout;
	editButtons->setSpacing(10);
	_btnSave   = makeButton(_editor, QString::fromUtf8("保存"), "primary", 72);
	_btnCancel = makeButton(_editor, QString::fromUtf8("取消"), "secondary", 72);
	editButtons->addWidget(_btnSave, 2);
	editButtons->addWidget(_btnCancel, 1);
	form->addSpacing(6);
	form->addLayout(editButtons);
	form->addStretch(1);

	scroll->setWidget(_editor);
	_pages->addWidget(scroll);

	root->addWidget(_pages, 1);

	// ---- action bar ----------------------------------------------------
	QHBoxLayout *actions = new QHBoxLayout;
	actions->setSpacing(10);
	_btnRun     = makeButton(this, QString::fromUtf8("运行"), "primary", 78);
	_btnConnect = makeButton(this, QString::fromUtf8("连接"), "primary", 78);
	_btnInsert  = makeButton(this, QString::fromUtf8("仅插入"), "secondary", 78);
	_btnAdd     = makeButton(this, QString::fromUtf8("新增"), "secondary", 78);
	_btnDelete  = makeButton(this, QString::fromUtf8("删除"), "danger", 78);

	actions->addWidget(_btnRun, 2);
	actions->addWidget(_btnConnect, 2);
	actions->addWidget(_btnInsert, 1);
	actions->addWidget(_btnAdd, 1);
	actions->addWidget(_btnDelete, 1);
	root->addLayout(actions);

	connect(_tabCommands, SIGNAL(clicked()), this, SLOT(onTabCommands()));
	connect(_tabSsh,      SIGNAL(clicked()), this, SLOT(onTabSsh()));
	connect(_cmdList, SIGNAL(itemActivated(QListWidgetItem*)),
	        this, SLOT(onCommandActivated(QListWidgetItem*)));
	connect(_sshList, SIGNAL(itemActivated(QListWidgetItem*)),
	        this, SLOT(onSshActivated(QListWidgetItem*)));
	connect(_btnRun,    SIGNAL(clicked()), this, SLOT(onRun()));
	connect(_btnInsert, SIGNAL(clicked()), this, SLOT(onInsert()));
	connect(_btnConnect, SIGNAL(clicked()), this, SLOT(onConnect()));
	connect(_btnAdd,    SIGNAL(clicked()), this, SLOT(onAdd()));
	connect(_btnDelete, SIGNAL(clicked()), this, SLOT(onDelete()));
	connect(_btnClose,  SIGNAL(clicked()), this, SLOT(onClose()));
	connect(_btnSave,   SIGNAL(clicked()), this, SLOT(onSave()));
	connect(_btnCancel, SIGNAL(clicked()), this, SLOT(onCancel()));

	setPage(PageCommands);
}

void CCommandPanel::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.fillRect(rect(), QColor(0x0b, 0x0e, 0x11, 250));
}

void CCommandPanel::popup()
{
	if (!parentWidget())
		return;
	setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
	reload();
	setPage(_page);
	show();
	raise();
	setFocus();
}

void CCommandPanel::dismiss()
{
	hide();
}

void CCommandPanel::setPage(int page)
{
	_page = page;
	_pages->setCurrentIndex(page);
	hideEditor();

	const bool cmds = (page == PageCommands);
	_btnRun->setVisible(cmds);
	_btnInsert->setVisible(cmds);
	_btnConnect->setVisible(!cmds);
	_btnDelete->setVisible(true);
	_btnAdd->setVisible(true);

	_tabCommands->setChecked(cmds);
	_tabSsh->setChecked(!cmds);

	_title->setText(cmds ? QString::fromUtf8("常用命令")
	                     : QString::fromUtf8("SSH 主机"));
	_subtitle->setText(cmds
	    ? QString::fromUtf8("点一行，再选 运行 / 仅插入")
	    : QString::fromUtf8("点一行，再选 连接（密码只做混淆保存）"));
}

void CCommandPanel::reload()
{
	reloadCommands();
	reloadSsh();
}

void CCommandPanel::reloadCommands()
{
	_cmdList->clear();
	const QStringList list = commandEntries();
	for (int i = 0; i < list.size(); ++i) {
		const QStringList parts = list.at(i).split('\t');
		if (parts.size() < 2)
			continue;
		QListWidgetItem *item = new QListWidgetItem(_cmdList);
		item->setData(Qt::DisplayRole, parts.at(0));
		item->setData(kDetailRole, parts.at(1));
		item->setData(Qt::UserRole, i);
	}
	if (_cmdList->count() == 0) {
		QListWidgetItem *item = new QListWidgetItem(_cmdList);
		item->setData(Qt::DisplayRole, QString::fromUtf8("还没有保存的命令"));
		item->setData(kDetailRole, QString::fromUtf8("点下面的 新增 添加一条"));
		item->setData(Qt::UserRole, -1);
	}
}

void CCommandPanel::reloadSsh()
{
	_sshList->clear();
	const QStringList list = sshEntries();
	for (int i = 0; i < list.size(); ++i) {
		const QStringList parts = list.at(i).split('\t');
		if (parts.size() < 4)
			continue;
		QString detail = QString("%1@%2:%3")
		                     .arg(parts.at(1), parts.at(2), parts.at(3));
		if (parts.size() > 4 && !parts.at(4).isEmpty())
			detail += QString::fromUtf8("  ·  key");
		QListWidgetItem *item = new QListWidgetItem(_sshList);
		item->setData(Qt::DisplayRole, parts.at(0));
		item->setData(kDetailRole, detail);
		item->setData(Qt::UserRole, i);
	}
	if (_sshList->count() == 0) {
		QListWidgetItem *item = new QListWidgetItem(_sshList);
		item->setData(Qt::DisplayRole, QString::fromUtf8("还没有保存的主机"));
		item->setData(kDetailRole, QString::fromUtf8("点下面的 新增 添加一台"));
		item->setData(Qt::UserRole, -1);
	}
}

QListWidget *CCommandPanel::currentList() const
{
	return _page == PageCommands ? _cmdList : _sshList;
}

void CCommandPanel::showEditor(bool on)
{
	if (!on) {
		hideEditor();
		_pages->setCurrentIndex(_page);
		return;
	}

	const bool cmds = (_page == PageCommands);
	_edCommand->setVisible(cmds);
	_edCommandLabel->setVisible(cmds);
	_edUser->setVisible(!cmds);
	_edUserLabel->setVisible(!cmds);
	_edHost->setVisible(!cmds);
	_edHostLabel->setVisible(!cmds);
	_edPort->setVisible(!cmds);
	_edPortLabel->setVisible(!cmds);
	_edKey->setVisible(!cmds);
	_edKeyLabel->setVisible(!cmds);
	_edPassword->setVisible(!cmds);
	_edPasswordLabel->setVisible(!cmds);

	_pages->setCurrentIndex(2);
	_edName->setFocus();
}

void CCommandPanel::hideEditor()
{
	if (_edName) { _edName->clear(); _edCommand->clear(); _edUser->clear();
	               _edHost->clear(); _edPort->clear(); _edKey->clear();
	               _edPassword->clear(); }
	if (_pages && _pages->currentIndex() == 2)
		_pages->setCurrentIndex(_page);
}

void CCommandPanel::onTabCommands() { setPage(PageCommands); }
void CCommandPanel::onTabSsh()      { setPage(PageSsh); }

void CCommandPanel::onCommandActivated(QListWidgetItem *item)
{
	if (!item) return;
	const QStringList list = commandEntries();
	const int idx = item->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= list.size()) return;
	const QStringList parts = list.at(idx).split('\t');
	_edName->setText(parts.value(0));
	_edCommand->setText(parts.value(1));
	_editIndex = idx;
	showEditor(true);
	_edName->setFocus();
}

void CCommandPanel::onSshActivated(QListWidgetItem *item)
{
	if (!item) return;
	const QStringList list = sshEntries();
	const int idx = item->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= list.size()) return;
	const QStringList parts = list.at(idx).split('\t');
	_edName->setText(parts.value(0));
	_edUser->setText(parts.value(1));
	_edHost->setText(parts.value(2));
	_edPort->setText(parts.value(3));
	_edKey->setText(parts.value(4));
	_edPassword->setText(decodeSecret(parts.value(5)));
	_editIndex = idx;
	showEditor(true);
	_edName->setFocus();
}

void CCommandPanel::onRun()
{
	QListWidgetItem *item = currentList()->currentItem();
	if (!item) return;
	const QStringList list = commandEntries();
	const int idx = item->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= list.size()) return;
	const QStringList parts = list.at(idx).split('\t');
	if (parts.size() < 2) return;
	emit commandChosen(parts.at(1), true);
}

void CCommandPanel::onInsert()
{
	QListWidgetItem *item = currentList()->currentItem();
	if (!item) return;
	const QStringList list = commandEntries();
	const int idx = item->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= list.size()) return;
	const QStringList parts = list.at(idx).split('\t');
	if (parts.size() < 2) return;
	emit commandChosen(parts.at(1), false);
}

void CCommandPanel::onConnect()
{
	QListWidgetItem *item = _sshList->currentItem();
	if (!item) return;
	const QStringList list = sshEntries();
	const int idx = item->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= list.size()) return;
	const QStringList parts = list.at(idx).split('\t');
	int port = parts.value(3).toInt();
	if (port <= 0) port = 22;
	emit sshChosen(parts.value(1), parts.value(2), port,
	               parts.value(4), decodeSecret(parts.value(5)));
}

void CCommandPanel::onAdd()
{
	hideEditor();
	_editIndex = -1;
	showEditor(true);
	_edPort->setText("22");
}

void CCommandPanel::onDelete()
{
	QListWidgetItem *item = currentList()->currentItem();
	if (!item) return;
	const int idx = item->data(Qt::UserRole).toInt();

	if (_page == PageCommands) {
		QStringList list = commandEntries();
		if (idx >= 0 && idx < list.size()) list.removeAt(idx);
		storeCommandEntries(list);
		reloadCommands();
	} else {
		QStringList list = sshEntries();
		if (idx >= 0 && idx < list.size()) list.removeAt(idx);
		storeSshEntries(list);
		reloadSsh();
	}
}

void CCommandPanel::onSave()
{
	const QString name = _edName->text().trimmed();
	if (name.isEmpty())
		return;

	const int editIdx = _editIndex;

	if (_page == PageCommands) {
		const QString cmd = _edCommand->text().trimmed();
		if (cmd.isEmpty())
			return;
		QStringList list = commandEntries();
		const QString entry = name + "\t" + cmd;
		if (editIdx >= 0 && editIdx < list.size())
			list[editIdx] = entry;
		else
			list.append(entry);
		storeCommandEntries(list);
		reloadCommands();
	} else {
		const QString host = _edHost->text().trimmed();
		if (host.isEmpty())
			return;
		QStringList list = sshEntries();
		int port = _edPort->text().trimmed().toInt();
		if (port <= 0) port = 22;
		const QString entry = QString("%1\t%2\t%3\t%4\t%5\t%6")
		    .arg(name, _edUser->text().trimmed(), host, QString::number(port),
		         _edKey->text().trimmed(), encodeSecret(_edPassword->text()));
		if (editIdx >= 0 && editIdx < list.size())
			list[editIdx] = entry;
		else
			list.append(entry);
		storeSshEntries(list);
		reloadSsh();
	}

	_editIndex = -1;
	hideEditor();
	_pages->setCurrentIndex(_page);
}

void CCommandPanel::onCancel()
{
	_editIndex = -1;
	hideEditor();
	_pages->setCurrentIndex(_page);
}

void CCommandPanel::onClose()
{
	hide();
}

// ---------------------------------------------------------------- storage --

QStringList CCommandPanel::commandEntries()
{
	QSettings settings;
	QStringList list = settings.value(kCommandsKey).toStringList();
	if (!settings.contains(kCommandsKey)) {
		// first run: a few generic entries; the user can delete them
		list << QString::fromUtf8("列出目录\tls -l")
		     << QString::fromUtf8("磁盘空间\tdf -h")
		     << QString::fromUtf8("进程\tps")
		     << QString::fromUtf8("网络地址\tifconfig")
		     << QString::fromUtf8("系统信息\tuname -a");
		settings.setValue(kCommandsKey, list);
	}
	return list;
}

QStringList CCommandPanel::sshEntries()
{
	QSettings settings;
	return settings.value(kSshKey).toStringList();
}

void CCommandPanel::storeCommandEntries(const QStringList &list)
{
	QSettings settings;
	settings.setValue(kCommandsKey, list);
}

void CCommandPanel::storeSshEntries(const QStringList &list)
{
	QSettings settings;
	settings.setValue(kSshKey, list);
}

// Not encryption: somebody with root or a backup can reverse it. It only keeps
// the password from being readable at a glance in the settings file.
QString CCommandPanel::encodeSecret(const QString &plain)
{
	if (plain.isEmpty())
		return QString();
	const QByteArray key("bgshellbb10");
	QByteArray raw = plain.toUtf8();
	for (int i = 0; i < raw.size(); ++i)
		raw[i] = raw.at(i) ^ key.at(i % key.size());
	return QString::fromLatin1(raw.toBase64());
}

QString CCommandPanel::decodeSecret(const QString &stored)
{
	if (stored.isEmpty())
		return QString();
	const QByteArray key("bgshellbb10");
	QByteArray raw = QByteArray::fromBase64(stored.toLatin1());
	for (int i = 0; i < raw.size(); ++i)
		raw[i] = raw.at(i) ^ key.at(i % key.size());
	return QString::fromUtf8(raw);
}
