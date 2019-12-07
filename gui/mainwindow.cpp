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
        ui->textEdit_Status->insertPlainText("No USB ports available.\nConnect a USB device and try again.\n");
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

void MainWindow::on_pushButton_reload_clicked()
{
    // Get all available COM Ports and store them in a QList.
    ui->comboBox_Interface->clear();
    ui->textEdit_Status->clear();
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
        ui->textEdit_Status->insertPlainText("No USB ports available.\nConnect a USB device and try again.\n");
    }
}

const double pi = 3.14159;
const int n_limit = 100;
static int n_max = 15;
static int count = 0;
static double node_pos [n_limit][3]; // Node_ID: y-axis // [1,:] xpos // [2,:] ypos // [3,:] # of outgoing edges
static QVector<double> alpha;
static double curr_pos [2];

void MainWindow::create_graph(QStringList InputList)
{
    // Paint edges and nodes
    QBrush greenBrush(Qt::green);
    QPen blackPen(Qt::black);
    blackPen.setWidth(1);

    curr_pos[0] = 0; // Initial x position
    curr_pos[1] = 0; // Initial y position
    static int curr_id = 0;

    //Random Generator
    count++;
    //QRandomGenerator randomGen;

    double x,y,x_offset,y_offset,len,new_length,new_alpha;
    double circle_radius=10*n_max;

    for(int i = 0; i<InputList.size()-1; i++) // Iterate all items in header "11:14:7:215:PAYLOAD"
    {
        int target_id = InputList[i].toInt();

        if (target_id >= n_max){ // doesn't check if header data is valid. invalid = 0. fix!?
            ui->textEdit_Status->insertPlainText("Invalid header id. Abort mission\n");
            break;
        }

        if ((abs(node_pos[target_id][0]) > 0) || (abs(node_pos[target_id][1]) > 0)){ // Target node already exists
            qDebug() << "Target node" << target_id << "already exists";
        }

        else{ // target node doesn't exist
            // increment outgoing edge counter
            node_pos[curr_id][2] += 1;

            // calculate new node position
            if (i==0){ // first ring
                x = cos(alpha.at(target_id));
                y = sin(alpha.at(target_id));
                x_offset = x*circle_radius;
                y_offset = y*circle_radius;
            }
            else{ // ring 2 and above
                len = sqrt(pow(curr_pos[0],2)+pow(curr_pos[1],2));
                new_length = len+circle_radius;
                new_alpha = atan2(curr_pos[1],curr_pos[0]) + (node_pos[curr_id][2]-1) / double(n_max*3*i) * 2*pi;
                x_offset = new_length*cos(new_alpha)-curr_pos[0];
                y_offset = new_length*sin(new_alpha)-curr_pos[1];
            }
            //int cnt = int(node_pos[target_id][2]);
            node_pos[target_id][0] = curr_pos[0] + x_offset;
            node_pos[target_id][1] = curr_pos[1] + y_offset;

            // add node circle
            mScene->addEllipse(node_pos[target_id][0]-10,node_pos[target_id][1]-10,20,20,blackPen,greenBrush);

            // add node id text
            QString node_id = QString::number(target_id);
            QGraphicsTextItem *text = mScene->addText(node_id);
            if (target_id < 10){ text->setPos(node_pos[target_id][0] +2-10, node_pos[target_id][1] -2-10); } // One-digit numbers format
            else{ text->setPos(node_pos[target_id][0] -3-10, node_pos[target_id][1] -2-10); } // Two-digit numbers format
        }

        // add node edges without cutting into node circle
        double x_delta = node_pos[target_id][0]-curr_pos[0];
        double y_delta = node_pos[target_id][1]-curr_pos[1];
        double normalizer = 10/sqrt(pow(x_delta,2)+pow(y_delta,2));
        double x1 = curr_pos[0]+normalizer*x_delta;
        double x2 = curr_pos[1]+normalizer*y_delta;
        double y1 = node_pos[target_id][0]-normalizer*x_delta;
        double y2 = node_pos[target_id][1]-normalizer*y_delta;

        mScene->addLine(x1,x2,y1,y2,blackPen); // add edge

        curr_pos[0] = node_pos[target_id][0]; // Update current x position
        curr_pos[1] = node_pos[target_id][1]; // Update current y position

        // update current id
        curr_id = target_id;
    }
}

void MainWindow::on_pushButton_create_clicked()
// test button for graph
{
    QString Input = ui->plainTextEdit_create->toPlainText();
    QStringList InputList = Input.split(":");
    MainWindow::create_graph(InputList);
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

void MainWindow::on_pushButton_explore_clicked()
{
    // create networkgraph
    mScene = new QGraphicsScene();
    ui->graphicsView_networkgraph->setScene( mScene );

    // disable notification text
    //ui->label_networkgraph->setVisible(0);

    // create root node
    QBrush redBrush(Qt::red);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    mScene->addEllipse(-10,-10,20,20,blackPen,redBrush);

    // enable node creation button
    ui->pushButton_create->setEnabled(true);

    // reset graph
    reset_graph();

    for (int i = 0; i < n_max; ++i) {
        alpha.insert(i, i / double(n_max) * 2 * pi); // devide circle angles into n_max equally spaced values
        qDebug() << alpha.at(i) << alpha.size();
    }
}

void MainWindow::on_pushButton_zoomin_clicked()
{
    ui->graphicsView_networkgraph->scale(1.2,1.2);
}

void MainWindow::on_pushButton_zoomout_clicked()
{
    ui->graphicsView_networkgraph->scale(0.8,0.8);
}

void MainWindow::reset_graph()
{
    // reset all values in node_pos
    for (int row = 0; row < n_max; ++row) // step through the rows in the array
        for (int col = 0; col < 3; ++col) // step through each element in the row
            node_pos[row][col] = 0;

    // reset alpha
    alpha.clear();

    // reset count
    count = 0;
}

void MainWindow::on_pushButton_setmax_clicked()
{
    QString Input = ui->plainTextEdit_setmax->toPlainText();
    if(Input.toInt()<n_limit){
        n_max = Input.toInt();
    }
    else{ui->textEdit_Status->insertPlainText("Please choose a value below " + Input + ".\n");}
}

void MainWindow::make_plot()
{
    //generate data:
    QVector<double> x(101),y(101);
    for (int i = 0; i < 101; ++i) {
        x[i] = i/50.0 - 1;
        y[i] = x[i]*x[i];
    }

    //create graph and assign data to it
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setData(x,y);
    ui->customPlot->xAxis->setLabel("x");
    ui->customPlot->yAxis->setLabel("y");
    ui->customPlot->xAxis->setRange(-1,1);
    ui->customPlot->yAxis->setRange(0,1);
    ui->customPlot->replot();
}

void MainWindow::on_pushButton_creategraph_clicked()
{
    make_plot();
}
