#include "mainwindow.h"

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
    double slider_value = ui->lcdNumber_slider->value();
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

const double pi = 3.14159;
const int n_max = 20;
static int count = 0;
static double node_pos [n_max][2]; // Node_ID: y-axis // [1,:] xpos // [2,:] ypos
static double curr_pos [2];

void MainWindow::on_pushButton_create_clicked()
//test button for graph
{
    QString strseq = ui->plainTextEdit_create->toPlainText();
    int intseq = strseq.toInt();

    // Paint edges and nodes
    QBrush greenBrush(Qt::green);
    QPen blackPen(Qt::black);
    blackPen.setWidth(1);

    curr_pos[0] = 0; // Initial x position
    curr_pos[1] = 0; // Initial y position

    //Random Generator
    count++;
    QRandomGenerator randomGen;

    if (intseq==0)
    {
        qDebug() << "Invalid Input Sequence";
    }
    else
    {
        for(int i = 0; i<strseq.length(); i++)
        {
            int target_id = intseq%10;
            randomGen.seed(quint32(count*i));
            double random_number = randomGen.generateDouble();
            double random_x = 100*cos(random_number*2*pi);
            double random_y = 100*sin(random_number*2*pi);

            if ((abs(node_pos[target_id][0]) > 0) || (abs(node_pos[target_id][1]) > 0)) // Target node already exists
            {
                qDebug() << "Target node" << target_id << "already exists";
            }
            else // Target node doesn't exist
            {
                // calculate new node position
                node_pos[target_id][0] = curr_pos[0] + random_x;
                node_pos[target_id][1] = curr_pos[1] + random_y;

                // add node circle
                mScene->addEllipse(node_pos[target_id][0]-10,node_pos[target_id][1]-10,20,20,blackPen,greenBrush);

                // add node id
                QString node_id = QString::number(target_id);
                QGraphicsTextItem *text = mScene->addText(node_id);
                if (target_id < 10){ text->setPos(node_pos[target_id][0] +2-10, node_pos[target_id][1] -2-10); } // One-digit numbers format
                else{ text->setPos(node_pos[target_id][0] -3-10, node_pos[target_id][0] -2-10); } // Two-digit numbers format
                //if (count%2 == 1){text->~QGraphicsTextItem();}
            }

            // add node edges without cutting into node circle
            double x_delta = node_pos[target_id][0]-curr_pos[0];
            double y_delta = node_pos[target_id][1]-curr_pos[1];
            double normalizer = 10/sqrt(pow(x_delta,2)+pow(y_delta,2));
            double x1 = curr_pos[0]+normalizer*x_delta;
            double x2 = curr_pos[1]+normalizer*y_delta;
            double y1 = node_pos[target_id][0]-normalizer*x_delta;
            double y2 = node_pos[target_id][1]-normalizer*y_delta;

            mScene->addLine(x1,x2,y1,y2,blackPen); // Add edge
            curr_pos[0] = node_pos[target_id][0]; // Move to new x position
            curr_pos[1] = node_pos[target_id][1]; // Move to new y position

            // qDebug() << "nodeprint:" << intseq%10;
            intseq = intseq/10; // for modulo operator to get single values

        }
    }
}

void MainWindow::on_pushButton_explore_clicked()
{
    // create networkgraph
    mScene = new QGraphicsScene();
    ui->graphicsView_networkgraph->setScene( mScene );

    // disable notification text
    ui->label_networkgraph->setVisible(0);

    // create root node
    QBrush redBrush(Qt::red);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    mScene->addEllipse(-10,-10,20,20,blackPen,redBrush);

    // enable node creation button
    ui->pushButton_create->setEnabled(true);

    // reset graph
    MainWindow::reset_graph();
}

void MainWindow::on_pushButton_zoomin_clicked()
{
    ui->graphicsView_networkgraph->scale(1.1,1.1);
}

void MainWindow::on_pushButton_zoomout_clicked()
{
    ui->graphicsView_networkgraph->scale(0.9,0.9);
}

void MainWindow::reset_graph()
{
    // reset all values in node_pos
    for (int row = 0; row < n_max; ++row) // step through the rows in the array
        for (int col = 0; col < 2; ++col) // step through each element in the row
            node_pos[row][col] = 0;

    // reset count
    count = 0;
}

