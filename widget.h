#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QIntValidator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_chooseFileButton_clicked();
    void socketConnected();
    void processSocketData();
    void process2();

private:
    Ui::Widget *ui;
    QRegularExpression *ipRegex;
    bool fileSet = false;
    QString fileName;
    QTcpServer *server;
    QTcpSocket *socket;
    QTcpSocket *abstractServerSocket;
    bool serverGotInfo = false;
    QString serverFileName;
};
#endif // WIDGET_H
