/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#ifndef MYSETTINGSWINDOW_H
#define MYSETTINGSWINDOW_H
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QVBoxLayout;

class CMySettingsWindow : public QWidget
{
	Q_OBJECT

public:
    CMySettingsWindow(QWidget *parent);
    int Init();

    QCheckBox *cbxProxyOn;
    QLabel *lblFontSample;
    QToolButton *btnClose;

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void on_cbFontSize_currentIndexChanged(const QString &);
    void on_cbHistory_currentIndexChanged(const QString &);
    void on_cbSshClient_currentIndexChanged(const QString &);
    void on_cbScrollBar_currentIndexChanged(const QString &);
    void on_btnClose_clicked();
    void onKeyBarAction();          // add / update / delete / reset
    void onKeyBarChecked(QListWidgetItem *item);   // tick toggled
    void onKeyBarToggled(bool on);
    void onKeyBarPicked(QListWidgetItem *item);

private:
    void buildKeyBarCard(QWidget *parent, QVBoxLayout *col);
    void keyBarReload();            // QSettings -> list
    void keyBarStore();             // list -> QSettings + live terminal
    void keyBarFillEditor(int row);

    QListWidget *kbList;
    QLineEdit   *kbLabel;
    QLineEdit   *kbPayload;
    QCheckBox   *kbEnabled;
};

#endif
