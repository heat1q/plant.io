#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qdebug.h>
#include "targetsettingswindow.h"
#include "networkgraph.h"
#include "node.h"
#include "edge.h"
#include <math.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Get all available COM Ports and store them in a QList.
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    // Read each element on the list, but
    // add only USB ports to the combo box.
    for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).portName.contains("USB")){
            ui->comboBox_Interface->addItem(ports.at(i).portName.toLocal8Bit().constData());
        }
    }
    // Show a hint if no USB ports were found.
    if (ui->comboBox_Interface->count() == 0){
        ui->textEdit_Status->insertPlainText("No USB ports available.\nConnect a USB device and try again.");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_pushButton_open_clicked()
{
    port.setQueryMode(QextSerialPort::EventDriven);
    port.setPortName("/dev/" + ui->comboBox_Interface->currentText());
    port.setBaudRate(BAUD115200);
    port.setFlowControl(FLOW_OFF);
    port.setParity(PAR_NONE);
    port.setDataBits(DATA_8);
    port.setStopBits(STOP_1);
    port.open(QIODevice::ReadWrite);

    if (!port.isOpen())
    {
        error.setText("Unable to open port!");
        error.show();
        return;
    }

    QObject::connect(&port, SIGNAL(readyRead()), this, SLOT(receive()));

    ui->pushButton_close->setEnabled(true);
    ui->pushButton_open->setEnabled(false);
    ui->comboBox_Interface->setEnabled(false);
}

void MainWindow::on_pushButton_close_clicked()
{
    if (port.isOpen())port.close();
    ui->pushButton_close->setEnabled(false);
    ui->pushButton_open->setEnabled(true);
    ui->comboBox_Interface->setEnabled(true);
}

void MainWindow::receive()
{

    static QString str;
        char ch;
        while (port.getChar(&ch))
        {
            str.append(ch);
            if (ch == '\n')     // End of line, start decoding
            {
                str.remove("\n", Qt::CaseSensitive);
                ui->textEdit_Status->append(str);

                // [Source 0x8edc, broadcast, RSSI -64]:	Temperature = 25000 mC
                if (str.contains("Temperature"))
                {

                    double value = 0.0;
                    QStringList list = str.split(QRegExp("\\s"));

                    qDebug() << "Str value: " << str;
                    if(!list.isEmpty()){
                        qDebug() << "List size " << list.size();
                        for (int i=0; i < list.size(); i++){
                            qDebug() << "List value "<< i <<" "<< list.at(i);
                            if (list.at(i) == "Temperature") {
                                value = list.at(i+2).toDouble();
                                //adjust to Degrees
                                value = value / 1000;
                                printf("%f\n",value);
                                ui->progressBar_light->setMaximum(40);
                            }
                        }
                    }

                    qDebug() << "Var value " << QString::number(value);
                    ui->lcdNumber_light->display(value);
                    ui->progressBar_light->setValue(static_cast<int>(value));
                }

                // [Source 0x8edc, broadcast, RSSI -64]:	Voltage [VDD] = 3276 mV ﾂ
                if (str.contains("Voltage [VDD] ="))
                {

                    double value2 = 0.0;
                    QStringList list = str.split(QRegExp("\\s"));

                    qDebug() << "Str value: " << str;
                    if(!list.isEmpty()){
                        qDebug() << "List size " << list.size();
                        for (int i=0; i < list.size(); i++){
                            qDebug() << "List value "<< i <<" "<< list.at(i);
                            if (list.at(i) == "[VDD]") {
                                value2 = list.at(i+2).toDouble();
                                //adjust to Degrees
                                value2 = value2 / 1000;
                                printf("%f\n",value2);
                            }
                        }
                    }

                    qDebug() << "Var value " << QString::number(value2);
                    ui->lcdNumber_voltage->display(value2);
                }

                this->repaint();    // Update content of window immediately
                str.clear();
            }
        }
}

void MainWindow::on_pushButton_clicked()
// Open new window
{
    TargetSettingsWindow page;
    page.setModal(true);
    page.exec();
}

void MainWindow::on_pushButton_send_clicked()
// Send text field message to port
{
    QString command;
    command = ui->plainTextEdit->toPlainText();
    command.prepend("0:");

    QByteArray byteArray = command.toLocal8Bit();
    byteArray.append('\n');
    port.write(byteArray);
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    ui->lcdNumber_slider->display(value);
}

void MainWindow::on_pushButton_send_threshold_t_clicked()
{
    double slider_value = (*ui).lcdNumber_slider->value();
    QString str = QString::number(slider_value);
    str.prepend("0:temp:");

    QByteArray byteArray = str.toLocal8Bit();
    byteArray.append('\n');
    port.write(byteArray);
}

void MainWindow::on_pushButton_send_threshold_ph_clicked()
{
    QString pH;
    pH = ui->plainTextEdit_pH->toPlainText();
    pH.prepend("0:ph:");

    QByteArray byteArray = pH.toLocal8Bit();
    byteArray.append('\n');
    port.write(byteArray);
}

void MainWindow::on_pushButton_reload_clicked()
{
    // Get all available COM Ports and store them in a QList.
    ui->comboBox_Interface->clear();
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    // Read each element on the list, but
    // add only USB ports to the combo box.
    for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).portName.contains("USB")){
            ui->comboBox_Interface->addItem(ports.at(i).portName.toLocal8Bit().constData());
        }
    }
    // Show a hint if no USB ports were found.
    if (ui->comboBox_Interface->count() == 0){
        ui->textEdit_Status->insertPlainText("No USB ports available.\nConnect a USB device and try again.");
    }
}

static int count = 0;

void MainWindow::on_pushButton_create_clicked()
//test button for graph
{
    static double x = 0;
    static double y = 0;

    QBrush greenBrush(Qt::green);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);

    //Random Generator
    count++;
    QRandomGenerator randomGen;
    randomGen.seed(quint32(count));
    double random_number = randomGen.generateDouble();
    double random_x = 50*cos(random_number*360);
    double random_y = 50*sin(random_number*360);
    x+=random_x;
    y+=random_y;
    //qDebug() << "Random number:" << random_number;

    mScene->addEllipse(x,y,20,20,blackPen,greenBrush);
    QString node_number = QString::number(count);
    QGraphicsTextItem *text = mScene->addText(node_number);

    if (count < 10){ text->setPos(x+2, y-2); } // One-digit numbers format
    else{ text->setPos(x-3, y-2); } // Two-digit numbers format

    //ui->graphicsView_networkgraph->scale(0.95,0.95);
    /*
    NetworkGraph networkGraph;
    QPointF pos(25,25);
    networkGraph.CreateNodeAtPosition( pos );
    */
}

void MainWindow::on_pushButton_explore_clicked()
{
    // create networkgraph
    mScene = new QGraphicsScene();
    ui->graphicsView_networkgraph->setScene( mScene );
    // disable notification text
    ui->label_networkgraph->setVisible(0);
    count = 0;
}
