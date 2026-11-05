/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#include <QtGui/QPalette>
#include <QtGui/QLabel>
#include <QtGui/QFrame>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include "qtermwidget.h"
#include "mymainwindow.h"
#include "mysettingswindow.h"
#include "uitheme.h"
#include "keybar.h"
#include "TerminalDisplay.h"

#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QLineEdit>
#include <QtGui/QApplication>

extern CMyMainWindow *mainWindow;
extern QString gTerminalFontFamily;

namespace {

// [ title / hint ............ control ]
QWidget *settingsRow(QWidget *parent, const QString &title,
                     const QString &hint, QWidget *control)
{
	QWidget *w = new QWidget(parent);
	QHBoxLayout *h = new QHBoxLayout(w);
	h->setContentsMargins(0, 4, 0, 4);
	h->setSpacing(16);

	QVBoxLayout *text = new QVBoxLayout;
	text->setContentsMargins(0, 0, 0, 0);
	text->setSpacing(3);

	QLabel *t = new QLabel(title, w);
	t->setStyleSheet(QString("QLabel { color: %1; font-size: 25px; }")
	                     .arg(UiTheme::hex(UiTheme::Text)));
	text->addWidget(t);

	if (!hint.isEmpty()) {
		QLabel *d = new QLabel(hint, w);
		d->setWordWrap(true);
		d->setStyleSheet(UiTheme::subtitleStyle());
		text->addWidget(d);
	}

	h->addLayout(text, 1);
	if (control) {
		control->setParent(w);
		h->addWidget(control, 0, Qt::AlignVCenter);
	}
	return w;
}

} // namespace

// The terminal lives deep inside the widget tree; the key strip has to change
// live, while the settings sheet is still open behind the editor.
static TerminalDisplay *liveTerminal()
{
	foreach (QWidget *w, QApplication::topLevelWidgets()) {
		TerminalDisplay *t = w->findChild<TerminalDisplay *>();
		if (t)
			return t;
	}
	return 0;
}

static QString keyBarRowText(const QString &label, const QString &payload)
{
	return QString::fromUtf8("%1   =   %2").arg(label, payload);
}

CMySettingsWindow::CMySettingsWindow(QWidget *parent) :
	QWidget(parent)
{
	setWindowModality(Qt::ApplicationModal);
	setObjectName(tr("wdgSettingsWindow"));
	setAutoFillBackground(false);
}

int CMySettingsWindow::Init(){
	QRect r = QApplication::desktop()->screenGeometry(0);
	int nWidth = r.width();
	int nHeight = r.height();
	this->setGeometry(0, 0, nWidth, nHeight);

	QSettings settings;

	QVBoxLayout *root = new QVBoxLayout(this);
	root->setContentsMargins(22, 18, 22, 20);
	root->setSpacing(16);

	root->addWidget(UiTheme::newHeader(this, QString::fromUtf8("设置"),
	                                  QString::fromUtf8("下滑菜单唤出 · 改动立即生效")));

	UiTheme::ScrollPane *scroll = new UiTheme::ScrollPane(this);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidgetResizable(true);
	root->addWidget(scroll, 1);

	QWidget *content = new QWidget(scroll);
	content->setObjectName(QString("panelPage"));
	content->setAutoFillBackground(false);
	QVBoxLayout *col = new QVBoxLayout(content);
	col->setContentsMargins(0, 0, 6, 0);
	col->setSpacing(16);
	scroll->setWidget(content);
	// after setWidget(), never before: that call re-enables the white backdrop
	UiTheme::styleScrollArea(scroll, UiTheme::Bg);

	// ---- 连接 -------------------------------------------------------
	QVBoxLayout *netBody = 0;
	QFrame *netCard = UiTheme::newCard(content, QString::fromUtf8("连接"), QString(), netBody);

	cbxProxyOn = new QCheckBox(QString::fromUtf8("启用"), netCard);
	cbxProxyOn->setChecked(settings.value("HttpProxyOn").toInt() != 0);
	netBody->addWidget(settingsRow(netCard, QString::fromUtf8("HTTP 代理"),
	                               QString::fromUtf8("装在 Work 分区或有严格防火墙时用；"
	                                                 "需系统设置里已填好代理"),
	                               cbxProxyOn));
	netBody->addWidget(settingsRow(netCard, QString::fromUtf8("保活"),
	                               QString::fromUtf8("30 秒一个心跳，熄屏不再掉线（固定）"),
	                               0));

	// The device ships its own OpenSSH, the app also carries a bundle.
	// main() reads data/sshclient while building PATH, i.e. before any of
	// this exists, so a change only lands on the next start.
	QComboBox *cbSshClient = new QComboBox(netCard);
	cbSshClient->setObjectName(tr("cbSshClient"));
	cbSshClient->setMaxVisibleItems(9);
	cbSshClient->addItem(QString::fromUtf8("自动"), QString("auto"));
	cbSshClient->addItem(QString::fromUtf8("系统 ssh"), QString("system"));
	cbSshClient->addItem(QString::fromUtf8("应用内置"), QString("bundled"));
	{
		QString cur("auto");
		QFile f(QString("data/sshclient"));
		if (f.open(QIODevice::ReadOnly)) {
			const QString v = QString::fromAscii(f.readAll()).trimmed().toLower();
			if (v == "system" || v == "bundled")
				cur = v;
		}
		const int idx = cbSshClient->findData(cur);
		if (idx >= 0)
			cbSshClient->setCurrentIndex(idx);
	}
	const bool hasSysSsh = QFile::exists("/usr/bin/ssh")
	                    || QFile::exists("/base/usr/bin/ssh");
	netBody->addWidget(settingsRow(netCard, QString::fromUtf8("SSH 客户端"),
	                               hasSysSsh
	                                   ? QString::fromUtf8("已找到系统 ssh；改完重启应用生效")
	                                   : QString::fromUtf8("系统里没找到 ssh，只能用内置；改完重启生效"),
	                               cbSshClient));
	col->addWidget(netCard);

	// ---- 外观 -------------------------------------------------------
	QVBoxLayout *lookBody = 0;
	QFrame *lookCard = UiTheme::newCard(content, QString::fromUtf8("外观"), QString(), lookBody);

	lblFontSample = new QLabel(QString::fromUtf8("Aa 中文"), lookCard);
	lblFontSample->setObjectName(tr("lblFontSample"));
	{
		QFont fontSample = QFont(QString("Courier New"), 6);
		fontSample.setFamily(gTerminalFontFamily.isEmpty() ? QString("Courier New")
		                                                   : gTerminalFontFamily);
		fontSample.setPixelSize(mainWindow->nFontSize);
		lblFontSample->setFont(fontSample);
		lblFontSample->setStyleSheet(QString("QLabel { color: %1; }")
		                                 .arg(UiTheme::hex(UiTheme::Accent)));
	}

	QComboBox *cbFontSize = new QComboBox(lookCard);
	cbFontSize->setObjectName(tr("cbFontSize"));
	cbFontSize->setMaxVisibleItems(9);
	for(int i=18; i < 52; i += 4){
	  cbFontSize->addItem(QString::number(i));
	  if (i == mainWindow->nFontSize)
	    cbFontSize->setCurrentIndex(cbFontSize->count()-1); // Current Font size
	}

	QWidget *fontControl = new QWidget(lookCard);
	QHBoxLayout *fontRow = new QHBoxLayout(fontControl);
	fontRow->setContentsMargins(0, 0, 0, 0);
	fontRow->setSpacing(12);
	fontRow->addWidget(lblFontSample);
	fontRow->addWidget(cbFontSize);

	lookBody->addWidget(settingsRow(lookCard, QString::fromUtf8("终端字体大小"),
	                                QString::fromUtf8("也可两指捏合缩放"),
	                                fontControl));

	QComboBox *cbHistory = new QComboBox(lookCard);
	cbHistory->setObjectName(tr("cbHistory"));
	cbHistory->setMaxVisibleItems(9);
	const int histVals[] = { 500, 1000, 2000, 5000, 10000 };
	const int nHistCur = settings.value("HistorySize", 2000).toInt();
	for (int i = 0; i < 5; ++i) {
	  cbHistory->addItem(QString::number(histVals[i]));
	  if (histVals[i] == nHistCur)
	    cbHistory->setCurrentIndex(cbHistory->count() - 1);
	}
	lookBody->addWidget(settingsRow(lookCard, QString::fromUtf8("回滚缓冲行数"),
	                                QString::fromUtf8("终端保留的历史行数"),
	                                cbHistory));

	// The panels scroll by dragging now, so the bar can shrink to a hairline.
	// Only the ink changes: the 26 px strip keeps taking a drag in every mode.
	QComboBox *cbScrollBar = new QComboBox(lookCard);
	cbScrollBar->setObjectName(tr("cbScrollBar"));
	cbScrollBar->setMaxVisibleItems(9);
	cbScrollBar->addItem(QString::fromUtf8("仅滚动时显示"), int(UiTheme::BarAutoHide));
	cbScrollBar->addItem(QString::fromUtf8("始终显示"), int(UiTheme::BarAlways));
	cbScrollBar->addItem(QString::fromUtf8("完全隐藏"), int(UiTheme::BarHidden));
	{
		const int idx = cbScrollBar->findData(int(UiTheme::scrollBarModeFromConfig()));
		if (idx >= 0)
			cbScrollBar->setCurrentIndex(idx);
	}
	lookBody->addWidget(settingsRow(lookCard, QString::fromUtf8("面板滚动条"),
	                                QString::fromUtf8("隐藏后仍可在面板里按住拖动"),
	                                cbScrollBar));

	col->addWidget(lookCard);

	buildKeyBarCard(content, col);

	QLabel *foot = new QLabel(QString::fromUtf8("快捷键与手势说明：CTRL 层按 ? 查看"), content);
	foot->setStyleSheet(UiTheme::subtitleStyle());
	foot->setAlignment(Qt::AlignHCenter);
	col->addWidget(foot);

	col->addStretch(1);

	btnClose = new QToolButton(this);
	btnClose->setObjectName(QString::fromUtf8("btnClose"));
	btnClose->setText(QString::fromUtf8("关闭"));
	btnClose->setMinimumHeight(76);
	btnClose->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	btnClose->setFocusPolicy(Qt::NoFocus);
	// objectName stays "btnClose" (connectSlotsByName needs it), so the
	// primary look is applied locally instead of via the sheet's #primary.
	btnClose->setStyleSheet(QString(
	    "QToolButton { background-color: %1; color: %2; border: none;"
	    "  border-radius: 16px; font-size: 26px; font-weight: bold; }"
	    "QToolButton:pressed { background-color: #00c24e; }")
	    .arg(UiTheme::hex(UiTheme::Accent), UiTheme::hex(UiTheme::AccentInk)));
	root->addWidget(btnClose);

	QMetaObject::connectSlotsByName(this);

	return 0;
}

void CMySettingsWindow::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), UiTheme::Bg);
}

extern QTermWidget *console;
void CMySettingsWindow::on_cbFontSize_currentIndexChanged(const QString & text){
    // User selected something from drop-down list
    int nSize = text.toInt();
	mainWindow->nFontSize = nSize;

	// Save new settings
	QSettings settings;
	settings.setValue("FontSize", nSize);

	// Create new Font
	QFont newfont = QFont(QString("Courier New"), 6);
	newfont.setFamily(gTerminalFontFamily.isEmpty() ? QString("Courier New")
	                                                : gTerminalFontFamily);
	newfont.setPixelSize(nSize);

    // Apply new Font/size to Sample Text
    lblFontSample->setFont(newfont);

    // Apply new Font/size to console
    if (console)
        console->setTerminalFont(newfont);
}

void CMySettingsWindow::on_cbHistory_currentIndexChanged(const QString &text){
    int nLines = text.toInt();
    if (nLines <= 0)
        return;

    QSettings settings;
    settings.setValue("HistorySize", nLines);

    // Takes effect on the running session too: only lines that fall out of
    // the new window are lost, growing the buffer keeps everything.
    if (console)
        console->setHistorySize(nLines);
}

void CMySettingsWindow::on_cbSshClient_currentIndexChanged(const QString &){
	QComboBox *cb = qobject_cast<QComboBox *>(sender());
	if (!cb)
		return;
	const QString pick = cb->itemData(cb->currentIndex()).toString();
	QFile f(QString("data/sshclient"));
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qDebug() << "settings: cannot write data/sshclient";
		return;
	}
	f.write(pick.toAscii());
	f.close();
}

void CMySettingsWindow::on_cbScrollBar_currentIndexChanged(const QString &){
	QComboBox *cb = qobject_cast<QComboBox *>(sender());
	if (!cb)
		return;
	const int mode = cb->itemData(cb->currentIndex()).toInt();

	QSettings settings;
	settings.setValue("ScrollBar", mode);

	// Live: every bar on screen repaints with the new look.
	UiTheme::setScrollBarMode(UiTheme::ScrollBarMode(mode));
}

void CMySettingsWindow::on_btnClose_clicked(){
    QEvent* event= new QEvent(QEvent::PlatformPanel);
    QApplication::sendEvent(this->parent(), event);
}

/* ---- custom key strip ------------------------------------------------ */

void CMySettingsWindow::buildKeyBarCard(QWidget *parent, QVBoxLayout *col)
{
	QVBoxLayout *body = 0;
	QFrame *card = UiTheme::newCard(parent, QString::fromUtf8("按键条"),
	                                QString::fromUtf8("勾选要显示的键；这些键也会出现在 SYM 面板底部"),
	                                body);

	kbEnabled = new QCheckBox(QString::fromUtf8("在终端底部常驻显示（不勾选则只在 SYM 面板里）"), card);
	kbEnabled->setChecked(QSettings().value(QString("KeyBarOn"), false).toBool());
	connect(kbEnabled, SIGNAL(toggled(bool)), this, SLOT(onKeyBarToggled(bool)));
	body->addWidget(kbEnabled);

	QLabel *tip = new QLabel(QString::fromUtf8(
	    "勾上的键才会出现在按键条里。条子只有一屏宽，勾太多会分页，所以只留常用的。"),
	    card);
	tip->setWordWrap(true);
	tip->setStyleSheet(UiTheme::subtitleStyle());
	body->addWidget(tip);

	kbList = new QListWidget(card);
	kbList->setMinimumHeight(560);
	kbList->setFocusPolicy(Qt::NoFocus);
	kbList->setStyleSheet(QString(
	    "QListWidget { background: #0d120f; color: %1; border: 1px solid #22302a;"
	    "  font-size: 22px; }"
	    "QListWidget::item { padding: 10px; }"
	    "QListWidget::item:selected { background: #00502a; color: #eafff2; }")
	    .arg(UiTheme::hex(UiTheme::Text)));
	connect(kbList, SIGNAL(itemChanged(QListWidgetItem *)),
	        this, SLOT(onKeyBarChecked(QListWidgetItem *)));
	connect(kbList, SIGNAL(itemClicked(QListWidgetItem *)),
	        this, SLOT(onKeyBarPicked(QListWidgetItem *)));
	body->addWidget(kbList);

	kbLabel = new QLineEdit(card);
	kbLabel->setPlaceholderText(QString::fromUtf8("自定义按钮名称，如 F1 / 保存 / ls"));
	kbLabel->setMinimumHeight(64);
	body->addWidget(kbLabel);

	kbPayload = new QLineEdit(card);
	kbPayload->setPlaceholderText(QString::fromUtf8(
	    "发送内容：\\e=Esc  \\t=Tab  \\r=回车  \\x02=Ctrl-B  \\x03=Ctrl-C，或普通文本"));
	kbPayload->setMinimumHeight(64);
	body->addWidget(kbPayload);

	QWidget *row = new QWidget(card);
	QHBoxLayout *h = new QHBoxLayout(row);
	h->setContentsMargins(0, 0, 0, 0);
	h->setSpacing(8);

	const char *acts[]  = { "add", "update", "delete", "reset" };
	const char *names[] = { "添加", "更新", "删除", "默认" };
	for (int i = 0; i < 4; ++i) {
		QToolButton *b = new QToolButton(row);
		b->setText(QString::fromUtf8(names[i]));
		b->setObjectName(QString::fromLatin1(acts[i]));
		b->setMinimumHeight(60);
		b->setFocusPolicy(Qt::NoFocus);
		connect(b, SIGNAL(clicked()), this, SLOT(onKeyBarAction()));
		h->addWidget(b, 1);
	}
	body->addWidget(row);

	QLabel *hint = new QLabel(QString::fromUtf8(
	    "添加：把上面两栏做成一个新按钮。删除：自带键只取消勾选，自定义键才真的移除。"
	    "默认：恢复成出厂的 12 个。转义：\\e Esc  \\t Tab  \\r 回车  \\x02 Ctrl-B  \\x03 Ctrl-C"),
	    card);
	hint->setWordWrap(true);
	hint->setStyleSheet(UiTheme::subtitleStyle());
	body->addWidget(hint);

	keyBarReload();
	col->addWidget(card);
}

// Rebuild the tick list: the built in pool first in its own order, then any
// button typed in by hand. The tick mark comes from what is really stored, so
// an untouched install simply shows twelve ticks and nothing else.
void CMySettingsWindow::keyBarReload()
{
	if (!kbList)
		return;

	kbList->blockSignals(true);
	kbList->clear();

	// loadItems() already knows about the older stored formats and the v4 trim,
	// and it is what the strip itself runs - so the ticks always match what is
	// actually on screen.
	QStringList remaining = CKeyBar::itemsToStrings(CKeyBar::loadItems());

	const QStringList poolEntries = CKeyBar::itemsToStrings(CKeyBar::pool());
	for (int i = 0; i < poolEntries.size(); ++i) {
		const QString entry = poolEntries.at(i);
		const int tab = entry.indexOf(QLatin1Char('\t'));
		QListWidgetItem *it = new QListWidgetItem(
		    keyBarRowText(entry.left(tab), entry.mid(tab + 1)), kbList);
		it->setData(Qt::UserRole, entry);
		it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
		it->setCheckState(remaining.removeAll(entry) > 0 ? Qt::Checked
		                                                : Qt::Unchecked);
	}

	// Whatever is left was made by the user; it has no slot in the pool, so it
	// sits below the stock keys and is on by definition.
	for (int i = 0; i < remaining.size(); ++i) {
		const QString entry = remaining.at(i);
		const int tab = entry.indexOf(QLatin1Char('\t'));
		QListWidgetItem *it = new QListWidgetItem(
		    keyBarRowText(entry.left(tab), entry.mid(tab + 1)), kbList);
		it->setData(Qt::UserRole, entry);
		it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
		it->setCheckState(Qt::Checked);
	}

	kbList->blockSignals(false);
}

void CMySettingsWindow::keyBarStore()
{
	QStringList list;
	for (int i = 0; i < kbList->count(); ++i) {
		QListWidgetItem *it = kbList->item(i);
		if (it->checkState() != Qt::Checked)
			continue;
		list << it->data(Qt::UserRole).toString();
	}

	QSettings settings;
	settings.setValue(QString::fromLatin1("KeyBarKeys"), list);
	// Without this an empty list would read back as "never configured" and the
	// strip would quietly come back with all the factory keys.
	settings.setValue(QString::fromLatin1("KeyBarSet"), true);

	// live: the strip is already on screen, so it has to repaint now
	TerminalDisplay *td = liveTerminal();
	if (td)
		td->reloadKeyBar();
}

void CMySettingsWindow::keyBarFillEditor(int row)
{
	if (!kbList || row < 0 || row >= kbList->count())
		return;
	const QString entry = kbList->item(row)->data(Qt::UserRole).toString();
	const int tab = entry.indexOf(QLatin1Char('\t'));
	kbLabel->setText(tab < 0 ? entry : entry.left(tab));
	kbPayload->setText(tab < 0 ? QString() : entry.mid(tab + 1));
}

void CMySettingsWindow::onKeyBarChecked(QListWidgetItem *)
{
	// A tick is the whole edit - write it through and let the strip redraw.
	keyBarStore();
}

void CMySettingsWindow::onKeyBarPicked(QListWidgetItem *item)
{
	if (!kbList || !item)
		return;
	keyBarFillEditor(kbList->row(item));
}

void CMySettingsWindow::onKeyBarAction()
{
	QToolButton *b = qobject_cast<QToolButton *>(sender());
	if (!b || !kbList)
		return;

	const QString act = b->objectName();
	const QString label = kbLabel->text().trimmed();
	const QString payload = kbPayload->text();

	if (act == QLatin1String("add")) {
		if (label.isEmpty() && payload.isEmpty())
			return;
		const QString entry = (label.isEmpty() ? QString("?") : label)
		                    + QLatin1Char('\t')
		                    + payload;
		kbList->blockSignals(true);
		QListWidgetItem *it = new QListWidgetItem(
		    keyBarRowText(entry.left(entry.indexOf(QLatin1Char('\t'))), payload),
		    kbList);
		it->setData(Qt::UserRole, entry);
		it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
		it->setCheckState(Qt::Checked);   // a button you just made is on
		kbList->setCurrentItem(it);
		kbList->blockSignals(false);
		keyBarStore();
		return;
	}

	if (act == QLatin1String("reset")) {
		QSettings settings;
		settings.remove(QString::fromLatin1("KeyBarKeys"));
		settings.remove(QString::fromLatin1("KeyBarSet"));
		keyBarReload();     // falls back to defaultItems()
		keyBarStore();      // and writes that down again
		return;
	}

	const int row = kbList->currentRow();
	if (row < 0)
		return;

	if (act == QLatin1String("delete")) {
		QListWidgetItem *it = kbList->item(row);
		const QString entry = it->data(Qt::UserRole).toString();
		if (CKeyBar::itemsToStrings(CKeyBar::pool()).contains(entry)) {
			// A stock key: untick it and leave the row, so it can come back.
			kbList->blockSignals(true);
			it->setCheckState(Qt::Unchecked);
			kbList->blockSignals(false);
		} else {
			delete kbList->takeItem(row);
		}
		keyBarStore();
		return;
	}

	if (act == QLatin1String("update")) {
		const QString entry = (label.isEmpty() ? QString("?") : label)
		                    + QLatin1Char('\t')
		                    + payload;
		kbList->blockSignals(true);
		QListWidgetItem *it = kbList->item(row);
		it->setData(Qt::UserRole, entry);
		it->setText(keyBarRowText(label, payload));
		it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
		it->setCheckState(Qt::Checked);
		kbList->blockSignals(false);
		keyBarStore();
		return;
	}
}

void CMySettingsWindow::onKeyBarToggled(bool on)
{
	QSettings settings;
	settings.setValue(QString("KeyBarOn"), on);

	TerminalDisplay *td = liveTerminal();
	if (td) {
		if (on)
			td->reloadKeyBar();     // pick up edits made a moment ago
		td->setKeyBarEnabled(on);
	}
}
