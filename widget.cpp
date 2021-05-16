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
    ui->portLineEdit->setText("3046");
    ui->logTextEdit->setReadOnly(true);
    ui->uploadProgressBar->setVisible(false);
    server = new QTcpServer();
    if(server->listen(QHostAddress::Any, 3046)) {
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
            socket = new QTcpSocket();
            socket->connectToHost(ui->ipLineEdit->text(), ui->portLineEdit->text().toInt());
            ui->logTextEdit->appendPlainText("Socket connecting...");
            if (socket->waitForConnected(1000))
                ui->logTextEdit->appendPlainText("Connected!");
            QDataStream out(socket);
            QFile file(fileName);
            if(file.open(QIODevice::ReadOnly)){
                ui->uploadProgressBar->setVisible(true);
                out << fileName.split("/").last().toLocal8Bit();
                QFileInfo fileInfo(fileName);
                ui->uploadProgressBar->setMaximum(fileInfo.size());
                ui->uploadProgressBar->setValue(0);
                while (!file.atEnd()) {
                    QByteArray temp = file.read(5000);
                    out << temp;
                    QEventLoop loop1;
                    QObject::connect(socket, &QTcpSocket::bytesWritten, &loop1, &QEventLoop::quit);
                    loop1.exec();
                    ui->uploadProgressBar->setValue(ui->uploadProgressBar->value()+temp.size());
                }
            }
            fileSet = false;
            ui->chooseFileButton->setText("Choose File");
            ui->fileNameLabel->setText("");
            ui->uploadProgressBar->setVisible(false);
            socket->disconnectFromHost();
            socket->deleteLater();
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
    QByteArray read;
    in >> read;
    serverFileName = read.data();
    QFile readToFile(serverFileName);
    if(readToFile.open(QIODevice::Append)){
        while (abstractServerSocket->bytesAvailable()) {
            QByteArray abcd;
            in >> abcd;
            readToFile.write(abcd);
        }
        QObject::connect(abstractServerSocket, &QTcpSocket::readyRead, this, &Widget::process2);
        QObject::connect(abstractServerSocket, &QTcpSocket::disconnected, this, &Widget::disconnected);
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
    }else{
        ui->logTextEdit->appendPlainText(readToFile.errorString());
    }
}

void Widget::disconnected(){
    ui->logTextEdit->appendPlainText("File saved, client disconnected.");
    ui->logTextEdit->appendPlainText("File: "+QDir::currentPath()+"/"+serverFileName);
    serverFileName.clear();

}
