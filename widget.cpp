#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget){

    ui->setupUi(this);
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    ipRegex = new QRegularExpression ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    ui->ipLineEdit->setValidator(new QRegularExpressionValidator(*ipRegex, this));
    ui->ipLineEdit->setText("127.0.0.1");
    ui->portLineEdit->setValidator(new QIntValidator(0,10000));
    ui->portLineEdit->setText("1251");
    ui->logTextEdit->setReadOnly(true);
    server = new QTcpServer();
    if(server->listen(QHostAddress::Any, 1251)) {
        ui->logTextEdit->appendPlainText("Server successfully started.");
        QObject::connect(server, &QTcpServer::newConnection, this, &Widget::socketConnected);
    }
    else ui->logTextEdit->appendPlainText("Cant start server, program works in client-only mode.");
}

Widget::~Widget(){
    delete ui;
}


void Widget::on_chooseFileButton_clicked(){
    if(fileSet){
        if(ipRegex->match(ui->ipLineEdit->text()).hasMatch()){
            ui->logTextEdit->appendPlainText("ip matched");
            socket = new QTcpSocket();
            ui->logTextEdit->appendPlainText("socket created");
            socket->connectToHost(ui->ipLineEdit->text(), ui->portLineEdit->text().toInt());
            ui->logTextEdit->appendPlainText("socket connecting...");
            if (socket->waitForConnected(1000))
                ui->logTextEdit->appendPlainText("Connected!");
            QDataStream out(socket);
            QFile file(fileName);
            if(file.open(QIODevice::ReadOnly)){
                out << fileName.split("/").last().toLocal8Bit();
                QByteArray fileRead = file.readAll();
                while (!fileRead.isEmpty()) {
                    QByteArray temp = fileRead.left(5000);
                    out << temp;
                    QEventLoop loop1;
                    QObject::connect(socket, &QTcpSocket::bytesWritten, &loop1, &QEventLoop::quit);
                    loop1.exec();
                    fileRead.remove(0, 5000);
                }
            }
            fileSet = false;
            ui->chooseFileButton->setText("Choose File");
        }else{
            ui->logTextEdit->appendPlainText("IP-address doesnt match the pattern");
        }
    }else{
        fileName = QFileDialog::getOpenFileName(this, "Выберите файл", QApplication::applicationDirPath());
        ui->fileNameLabel->setText(fileName.split("/").last());
        ui->logTextEdit->appendPlainText("Selected file: "+fileName);
        ui->chooseFileButton->setText("Send File");
        fileSet = true;
    }
}

void Widget::socketConnected(){
    ui->logTextEdit->appendPlainText("New Connection");
    abstractServerSocket = server->nextPendingConnection();
    processSocketData();
}

void Widget::processSocketData(){
    QEventLoop loop1;
    QObject::connect(abstractServerSocket, &QTcpSocket::readyRead, &loop1, &QEventLoop::quit);
    loop1.exec();
    QObject::disconnect(abstractServerSocket);
    QDataStream in(abstractServerSocket);
    if(!serverGotInfo){
        QByteArray read;
        in >> read;
        serverFileName = read.data();
        ui->logTextEdit->appendPlainText(read);
        serverGotInfo = true;
    }
    QFile readToFile(serverFileName);
    if(readToFile.open(QIODevice::Append)){
        while (abstractServerSocket->bytesAvailable()) {
            QByteArray abcd;
            in >> abcd;
            readToFile.write(abcd);
        }
        ui->logTextEdit->appendPlainText("Data read.");
        QObject::connect(abstractServerSocket, &QTcpSocket::readyRead, this, &Widget::process2);
    }else{
        ui->logTextEdit->appendPlainText(readToFile.errorString());
    }
}

void Widget::process2(){
    QDataStream in(abstractServerSocket);
    QFile readToFile(serverFileName);
    if(readToFile.open(QIODevice::Append)){
        while (abstractServerSocket->bytesAvailable()) {
            QByteArray abcd;
            in >> abcd;
            readToFile.write(abcd);
        }
        ui->logTextEdit->appendPlainText("Data read.");
    }else{
        ui->logTextEdit->appendPlainText(readToFile.errorString());
    }
}
