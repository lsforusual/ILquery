#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
//#include <QSqlQueryModel>
#include <QSqlError>
#include <QSqlRecord>
#include <QCompleter>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool getCid(const QString& cation, QString& cid);
    bool getAid(const QString& anion, QString& aid);
    bool getBasicInfo(QString& cid,QString& aid);
    bool getThermoInfo(QString& cid,QString&aid);
    bool getECInfo(QString& cid,QString&aid);
    void setCompleter();

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase m_db;
    QSqlQuery *m_query,*m_query_ref;
    QStringList m_queryList;
};

#endif // MAINWINDOW_H
