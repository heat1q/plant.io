/**
 * @file mainwindow.cpp
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief
 * @version 1.0
 * @date 2019-12-12
 *
 * @copyright Copyright (c) 2019
 *
 */
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
        print("No USB ports available.\nConnect a USB device and try again.\n");
    }

    QPalette p = ui->textEdit_Status->palette(); // define palette for textEdit
    p.setColor(QPalette::Base, Qt::black); // set background color
    p.setColor(QPalette::Text, Qt::white); // set text color
    ui->textEdit_Status->setPalette(p); // change textedit palette
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

const double pi = 3.14159;
const int n_limit = 100;
static int n_max = 15;
static double node_pos [n_limit][3]; // Node_ID: y-axis // [1,:] xpos // [2,:] ypos // [3,:] # of outgoing edges
static double curr_pos [2];
static QVector<double> alpha;
static QVector<int> ack_queue;

// retransmissions / ACKs
static QListWidget* re_listWidget;
static QString re_requestType;
static int num_retransmissions;
const static int ack_timer = n_max * 500; // waiting time until ACK check in ms

// outside function for plot() to access the graphs
static QCPAxisRect *TempAxisRect;
static QCPAxisRect *HumAxisRect;
static QCPAxisRect *LightAxisRect;
static QCPLegend *legend;
static QVector<int> existingLegendIDs;

void MainWindow::create_graph(QStringList InputList) // Add a route to the graph
{
    // Paint edges and nodes
    QBrush fillBrush(Qt::cyan);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    QPen updatePen(Qt::green); // Color for updated route
    updatePen.setWidth(2);

    curr_pos[0] = 0; // Initial x position
    curr_pos[1] = 0; // Initial y position
    static int curr_id = 0;
    bool isUpdate = false;

    double x,y,x_offset,y_offset,len,new_length,new_alpha;
    double circle_radius=100+4.20*n_max;

    // first if condition blocks the updated graph color so it doesn't mistrigger if e.g. 1:2:3 comes before 1:2 in exploration phase
    int index_last = InputList.first().toInt(); // .first due to reverse order!
    if (ui->pushButton_Explore->isEnabled() && ((abs(node_pos[index_last][0]) > 0) || (abs(node_pos[index_last][1]) > 0))){ // Route has been updated
        isUpdate = true;
    }

    for(int i = InputList.size()-1; i>=0; i--) // Iterate all items in header "11:14:7:215"
    {
        int target_id = InputList[i].toInt();

        if (target_id >= n_max){
            print("Invalid header id. Aborting graph creation!\n");
            return;
        }

        if ((abs(node_pos[target_id][0]) > 0) || (abs(node_pos[target_id][1]) > 0)){ // Target node already exists
            //qDebug() << "Target node" << target_id << "already exists";
        }
        else{ // target node doesn't exist
            // calculate new node position
            if (i==InputList.size()-1){ // first ring
                x = cos(alpha.at(target_id));
                y = sin(alpha.at(target_id));
                x_offset = x*circle_radius;
                y_offset = y*circle_radius;
            }
            else{ // ring 2 and above
                len = sqrt(pow(curr_pos[0],2)+pow(curr_pos[1],2));
                new_length = len + circle_radius;
                new_alpha = atan2(curr_pos[1],curr_pos[0]) + (node_pos[curr_id][2]-0.5) / double(2.5*(InputList.size()-i)*n_max) * 2*pi;
                x_offset = new_length*cos(new_alpha)-curr_pos[0];
                y_offset = new_length*sin(new_alpha)-curr_pos[1];
            }
            //int cnt = int(node_pos[target_id][2]);
            node_pos[target_id][0] = curr_pos[0] + x_offset;
            node_pos[target_id][1] = curr_pos[1] + y_offset;

            // increment outgoing edge counter
            node_pos[curr_id][2] += 1;

            // add node circle
            mScene->addEllipse(node_pos[target_id][0]-10,node_pos[target_id][1]-10,20,20,blackPen,fillBrush);

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

        if (isUpdate){
            mScene->addLine(x1,x2,y1,y2,updatePen); // add edge
        } else{
            mScene->addLine(x1,x2,y1,y2,blackPen); // add edge
        }

        curr_pos[0] = node_pos[target_id][0]; // Update current x position
        curr_pos[1] = node_pos[target_id][1]; // Update current y position

        // update current id
        curr_id = target_id;
    }

    on_pushButton_Center_clicked();
}

void MainWindow::reset_graph() // Reset the graph
{
    // reset all values in node_pos
    for (int row = 0; row < n_max; ++row) // step through the rows in the array
        for (int col = 0; col < 3; ++col) // step through each element in the row
            node_pos[row][col] = 0;

    // reset alpha
    alpha.clear();

    // clear ack_queue
    ack_queue.clear();
}

void MainWindow::send2port(QString input) // Send message to port
{
    QByteArray byteArray = input.toLocal8Bit();
    byteArray.append('\n');
    if (port.write(byteArray)==-1)
    {
        print("QextSerialPort: device not open!\n");
    }
}

void MainWindow::print(QString msg) // Print a message in GUI console
{
    ui->textEdit_Status->moveCursor (QTextCursor::End);
    ui->textEdit_Status->insertPlainText (msg);
    ui->textEdit_Status->moveCursor (QTextCursor::End);
}

void MainWindow::plot(int type, QStringList data, QString id) // ID : SENSOR_DATA : TYPE : DATA
{
    QCustomPlot* plot = ui->customPlot;
    QCPGraph* target_graph;

    // Assign the correct graph according to the input type
    if (type == 1){ // TEMPERATURE
        target_graph = plot->addGraph(TempAxisRect->axis(QCPAxis::atBottom), TempAxisRect->axis(QCPAxis::atLeft));
    }
    else if (type == 2) { // HUMIDITY
        target_graph = plot->addGraph(HumAxisRect->axis(QCPAxis::atBottom), HumAxisRect->axis(QCPAxis::atLeft));
    }
    else if (type == 3){ // LIGHT
        target_graph = plot->addGraph(LightAxisRect->axis(QCPAxis::atBottom), LightAxisRect->axis(QCPAxis::atLeft));
    }
    else { // INVALID
        print("WARNING: Invalid sensor data type recieved!");
        return;
    }

    // Extract the data from the input sequence
    int num_elements = data.count() - 2;
    QVector<double> x,y;
    for (int i = 0; i < num_elements; ++i) {
        x.append(i);
        y.append(data[i+2].toDouble());
    }

    // Plot the data
    target_graph->setName("ID#"+id);
    target_graph->setData(x,y);
    target_graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black), QBrush(Qt::white), 3));
    int i = id.toInt();
    target_graph->setPen(QPen(QColor((255+i*100)%255, (255+i*200)%255, (255+i*300)%255), 2));
    target_graph->rescaleAxes();

    if (!existingLegendIDs.contains(i)){ // legend for id not yet set
        legend->addItem(new QCPPlottableLegendItem(legend, target_graph));
        existingLegendIDs.append(i);
    }

    // Rescale and replot
    plot->rescaleAxes();
    plot->replot();
}

void MainWindow::receive() // QObject::connect(&port, SIGNAL(readyRead()), this, SLOT(receive()));
{
    static QString str;
    char ch;
    while (port.getChar(&ch))
    {
        str.append(ch);
        //msg.append(ch);
        if (ch == '<'){
            str.clear();
        }
        else if (ch == '>') {
            str.remove(">");
            QStringList data = str.split(":");

            // ACK
            QString id = data[0];
            print("Received ACK / DATA from Mote: " + id);
            ack_queue.removeAll(id.toInt());

            if (data[1] == "route"){
                data.removeFirst();
                data.removeFirst();
                create_graph(data); // create visualization of route which also registers the ID's as valid targets
            }
            else if (data[1] == "sensor_data"){ // ID : SENSOR_DATA : TYPE : DATA
                data.removeAt(0);
                data.removeAt(0);
                int count = data.count();
                for (int i = 0; i < 3; ++i) {
                    QStringList currData;
                    for (int j = i*count/3; j < (i+1)*count/3; ++j) {
                        currData.append(data[j]);
                    }
                    plot(i+1, currData, id);
                }
            }
            else if (data[1] == "th") {
                print("Thresholds from Zolertia™ Re-Mote ID" + data[0] + ":\n"
                        + "T_Min:" + data[2] + " | T_Max:" + data[3]
                        + " | H_Min:" + data[4] + " | H_Max:" + data[5]
                        + " | L_Min:" + data[6] + " | L_Max:" + data[7] + "\n");
            }

            str.clear();
        }
        else if (ch == '\n')     // End of line, start decoding
        {
            print(str);
            this->repaint();    // Update content of window immediately
            str.clear();
        }
    }
}

void MainWindow::on_pushButton_Open_clicked()
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

    ui->pushButton_Close->setEnabled(true);
    ui->pushButton_Open->setEnabled(false);
    ui->comboBox_Interface->setEnabled(false);
}

void MainWindow::on_pushButton_Close_clicked()
{
    if (port.isOpen())port.close();
    ui->pushButton_Close->setEnabled(false);
    ui->pushButton_Open->setEnabled(true);
    ui->comboBox_Interface->setEnabled(true);
}

void MainWindow::on_pushButton_Reload_clicked()
{
    // Get all available COM Ports and store them in a QList.
    ui->comboBox_Interface->clear();
    //ui->textEdit_Status->clear();
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
        print("No USB ports available.\nConnect a USB device and try again.\n");
    }
}

void MainWindow::on_pushButton_Explore_clicked()
{
    send2port("0:init:"); // Initialize network exploration on mote

    // create networkgraph
    mScene = new QGraphicsScene();
    ui->graphicsView_Networkgraph->setScene( mScene );

    // create root node
    QBrush redBrush(Qt::red);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    mScene->addEllipse(-10,-10,20,20,blackPen,redBrush);

    // reset graph
    reset_graph();

    // devide circle angles into n_max equally spaced values
    for (int i = 0; i < n_max; ++i) {
        alpha.insert(i, i / double(n_max) * 2 * pi);
    }

    // Disable the button for 8000ms -> also blocks the updated graph color so it doesn't mistrigger if e.g. 1:2:3 comes before 1:2 in exploration phase
    ui->pushButton_Explore->setEnabled(false);
    QTimer::singleShot(8000, this, SLOT(enableButton())); // first entry is time in ms
}

void MainWindow::enableButton() // enable the explore network button
{
    ui->pushButton_Explore->setEnabled(true);
}

void MainWindow::on_pushButton_ZoomIn_clicked()
{
    ui->graphicsView_Networkgraph->scale(1.2,1.2);
}

void MainWindow::on_pushButton_ZoomOut_clicked()
{
    ui->graphicsView_Networkgraph->scale(0.8,0.8);
}

void MainWindow::on_pushButton_SetMax_clicked()
{
    int Input = qRound(ui->doubleSpinBox_SetMax->value());
    if(Input<n_limit){
        n_max = Input;
    }
    else{print("Please choose a value below " + QString::number(n_limit) + ".\n");}
}


void MainWindow::send2selection(QListWidget* listWidget, QString requestType)
{
    ack_queue.clear();
    num_retransmissions = 1;

    QList<QListWidgetItem *> selection = listWidget->selectedItems();
    if ((selection.count() > 0) && (port.isOpen())){

        for (int i = 0; i < selection.count(); ++i) {
            QListWidgetItem listItem = *selection.at(i);
            QString id;
            if (listItem.text().contains("GUI")){
                id = "0"; // GUI TARGET ID
            }
            else {
                id = listItem.text().split("ID")[1];
            }

            // Add relevant id's to acknowledgement queue
            if (id != "0"){ // non GUI node id
                ack_queue.append(id.toInt());
            }

            // REQUEST TYPES
            if (requestType == "Get routing table"){
                QString cmd = id + ":rt";
                send2port(cmd);
            }
            else if (requestType == "Send temperature threshold") {
                double minTemp = ui->doubleSpinBox_MinTemp->value();
                double maxTemp = ui->doubleSpinBox_MaxTemp->value();
                QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp) + ":-1:-1:-1:-1";
                send2port(cmd);
            }
            else if (requestType == "Send humidity threshold") {
                double minHum = ui->doubleSpinBox_MinHum->value();
                double maxHum = ui->doubleSpinBox_MaxHum->value();
                QString cmd = id + ":set_th:-1:-1:" + QString::number(minHum) + ":" + QString::number(maxHum) + ":-1:-1";
                send2port(cmd);
            }
            else if (requestType == "Send light threshold") {
                double minLight = ui->doubleSpinBox_MinLight->value();
                double maxLight = ui->doubleSpinBox_MaxLight->value();
                QString cmd = id + ":set_th:-1:-1:-1:-1:" + QString::number(minLight) + ":" + QString::number(maxLight);
                send2port(cmd);
            }
            else if (requestType == "Send all thresholds") {
                double minTemp = ui->doubleSpinBox_MinTemp->value();
                double maxTemp = ui->doubleSpinBox_MaxTemp->value();
                double minHum = ui->doubleSpinBox_MinHum->value();
                double maxHum = ui->doubleSpinBox_MaxHum->value();
                double minLight = ui->doubleSpinBox_MinLight->value();
                double maxLight = ui->doubleSpinBox_MaxLight->value();
                QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp)
                        + ":" + QString::number(minHum) + ":" + QString::number(maxHum)
                        + ":" + QString::number(minLight) + ":" + QString::number(maxLight);
                send2port(cmd);
            }
            else if (requestType == "Get all thresholds") {
                QString cmd = id + ":get_th";
                send2port(cmd);
            }
            else if (requestType == "Get sensor data") {
                send2port(id + ":get_data:");
            }
            else if (requestType == "Set LED color") {
                int LED_colorValue = 0;
                if (ui->checkBox_R->isChecked()){
                    LED_colorValue += 1;
                }
                if (ui->checkBox_G->isChecked()){
                    LED_colorValue += 2;
                }
                if (ui->checkBox_B->isChecked()){
                    LED_colorValue += 4;
                }
                send2port(id + ":led:" + QString::number(LED_colorValue));
            }
        }
        // Verify all acknowledgements after a certain period of time and retransmit the one that havent received an ACK yet
        re_listWidget = listWidget;
        re_requestType = requestType;
        if (ack_queue.count() != 0){
            QTimer::singleShot(ack_timer, this, SLOT(resend2selection())); // first entry is time in ms
        }
    }
    else {
        print("No target motes selected OR no port connection!\n");
    }
}

void MainWindow::resend2selection()
{
    if (ack_queue.count() > 0){ // there are still unfulfilled requests
        if (num_retransmissions > 2){
            print("One or more routes are unresponsive. Initiating re-exploration of network!\n");
            on_pushButton_Explore_clicked();
            return;
        }

        // Verify all acknowledgements after a certain period of time and retransmit the one that havent received an ACK yet
        QTimer::singleShot(ack_timer, this, SLOT(resend2selection())); // first entry is time in ms
        num_retransmissions += 1;

        for (int i = 0; i < ack_queue.count(); ++i) { // Retransmit all unfinished requests
            QString id = QString::number(ack_queue[i]);

            // REQUEST TYPES
            if (re_requestType == "Get routing table"){
                QString cmd = id + ":rt";
                send2port(cmd);
            }
            else if (re_requestType == "Send temperature threshold") {
                double minTemp = ui->doubleSpinBox_MinTemp->value();
                double maxTemp = ui->doubleSpinBox_MaxTemp->value();
                QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp) + ":-1:-1:-1:-1";
                send2port(cmd);
            }
            else if (re_requestType == "Send humidity threshold") {
                double minHum = ui->doubleSpinBox_MinHum->value();
                double maxHum = ui->doubleSpinBox_MaxHum->value();
                QString cmd = id + ":set_th:-1:-1:" + QString::number(minHum) + ":" + QString::number(maxHum) + ":-1:-1";
                send2port(cmd);
            }
            else if (re_requestType == "Send light threshold") {
                double minLight = ui->doubleSpinBox_MinLight->value();
                double maxLight = ui->doubleSpinBox_MaxLight->value();
                QString cmd = id + ":set_th:-1:-1:-1:-1:" + QString::number(minLight) + ":" + QString::number(maxLight);
                send2port(cmd);
            }
            else if (re_requestType == "Send all thresholds") {
                double minTemp = ui->doubleSpinBox_MinTemp->value();
                double maxTemp = ui->doubleSpinBox_MaxTemp->value();
                double minHum = ui->doubleSpinBox_MinHum->value();
                double maxHum = ui->doubleSpinBox_MaxHum->value();
                double minLight = ui->doubleSpinBox_MinLight->value();
                double maxLight = ui->doubleSpinBox_MaxLight->value();
                QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp)
                        + ":" + QString::number(minHum) + ":" + QString::number(maxHum)
                        + ":" + QString::number(minLight) + ":" + QString::number(maxLight);
                send2port(cmd);
            }
            else if (re_requestType == "Get all thresholds") {
                QString cmd = id + ":get_th";
                send2port(cmd);
            }
            else if (re_requestType == "Get sensor data") {
                send2port(id + ":get_data:");
            }
            else if (re_requestType == "Set LED color") {
                int LED_colorValue = 0;
                if (ui->checkBox_R->isChecked()){
                    LED_colorValue += 1;
                }
                if (ui->checkBox_G->isChecked()){
                    LED_colorValue += 2;
                }
                if (ui->checkBox_B->isChecked()){
                    LED_colorValue += 4;
                }
                send2port(id + ":led:" + QString::number(LED_colorValue));
            }
        }
    }
    else {
        print("All acknowledgements received!\n");
    }
}

void MainWindow::on_pushButton_GetSensorData_clicked()
{
    existingLegendIDs.clear();
    QCustomPlot* plot = ui->customPlot;
    plot->plotLayout()->clear();
    plot->clearItems();
    plot->clearGraphs();
    plot->plotLayout()->simplify();

    TempAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(0, 0, TempAxisRect);
    HumAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(1, 0, HumAxisRect);
    LightAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(2, 0, LightAxisRect);

    // Add graphs to name the key and value axis
    QCPGraph *TempGraph = plot->addGraph(TempAxisRect->axis(QCPAxis::atBottom), TempAxisRect->axis(QCPAxis::atLeft));
    QCPGraph *HumGraph = plot->addGraph(HumAxisRect->axis(QCPAxis::atBottom), HumAxisRect->axis(QCPAxis::atLeft));
    QCPGraph *LightGraph = plot->addGraph(LightAxisRect->axis(QCPAxis::atBottom), LightAxisRect->axis(QCPAxis::atLeft));
    // Name the key and value axis
    TempGraph->keyAxis()->setLabel("Sample");
    TempGraph->valueAxis()->setLabel("Temp in mC");
    HumGraph->keyAxis()->setLabel("Sample");
    HumGraph->valueAxis()->setLabel("Humidity in %");
    LightGraph->keyAxis()->setLabel("Sample");
    LightGraph->valueAxis()->setLabel("Light in Lumen");

    // Add the legend at the top right position
    legend = new QCPLegend;
    plot->axisRect()->insetLayout()->addElement(legend, Qt::AlignTop|Qt::AlignRight);

    // Set margins for all (3) figures
    for (int i = 0; i < 3; ++i) {
        plot->axisRect(i)->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom);
        plot->axisRect(i)->setMargins(QMargins(0,0,100,0));
    }
    // Settings for custom legend
    plot->axisRect(0)->insetLayout()->setInsetPlacement(0, QCPLayoutInset::ipFree);
    plot->axisRect(0)->insetLayout()->setInsetRect(0, QRectF(1,0,0,0));
    plot->setAutoAddPlottableToLegend(false);

    send2selection(ui->listWidget_Tab2, "Get sensor data");

    // Set the plots to grid mode
    QList<QCPAxis*> allAxes;
    allAxes << TempAxisRect->axes() << HumAxisRect->axes() << LightAxisRect->axes();
    foreach (QCPAxis *axis, allAxes)
    {
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");
    }

    // Apply the configuration changes by replotting
    plot->replot();
}

void MainWindow::on_pushButton_Refresh_Tab2_clicked()
{
    ui->listWidget_Tab2->clear();

    if (port.isOpen()){
        QString message = "Zolertia™ GUI Re-Mote";
        QListWidgetItem *listItem = new QListWidgetItem(
                    QIcon(QCoreApplication::applicationDirPath() + "/../gui/resource/remote.png"), message, ui->listWidget_Tab2);
        ui->listWidget_Tab2->addItem(listItem); // ADD GUI MOTE OPTION MANUALLY
    }

    for (int i = 0; i < n_max; ++i) {
        if ((int(node_pos[i][0])!=0)||(int(node_pos[i][1])!=0)){
            QString message = "Zolertia™ Re-Mote ID" + QString::number(i);
            QListWidgetItem *listItem = new QListWidgetItem(
                        QIcon(QCoreApplication::applicationDirPath() + "/../gui/resource/remote.png"), message, ui->listWidget_Tab2);
            ui->listWidget_Tab2->addItem(listItem);
        }
    }
}

void MainWindow::on_pushButton_SelectAll_Tab2_clicked()
{
    ui->listWidget_Tab2->selectAll();
}

void MainWindow::on_pushButton_UnselectAll_Tab2_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget_Tab2->selectedItems();
    for (int i = 0; i < selection.count(); i++) {
        ui->listWidget_Tab2->setItemSelected(selection[i], false);
    }
}

void MainWindow::on_pushButton_Refresh_Tab3_clicked()
{
    ui->listWidget_Tab3->clear();

    if (port.isOpen()){
        QString message = "Zolertia™ GUI Re-Mote";
        QListWidgetItem *listItem = new QListWidgetItem(
                    QIcon(QCoreApplication::applicationDirPath() + "/../gui/resource/remote.png"), message, ui->listWidget_Tab3);
        ui->listWidget_Tab3->addItem(listItem); // ADD GUI MOTE OPTION MANUALLY
    }

    for (int i = 0; i < n_max; ++i) {
        if ((int(node_pos[i][0])!=0)||(int(node_pos[i][1])!=0)){
            QString message = "Zolertia™ Re-Mote ID" + QString::number(i);
            QListWidgetItem *listItem = new QListWidgetItem(
                        QIcon(QCoreApplication::applicationDirPath() + "/../gui/resource/remote.png"), message, ui->listWidget_Tab3);
            ui->listWidget_Tab3->addItem(listItem);
        }
    }
}

void MainWindow::on_pushButton_SelectAll_Tab3_clicked()
{
    ui->listWidget_Tab3->selectAll();
}

void MainWindow::on_pushButton_UnselectAll_Tab3_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget_Tab3->selectedItems();
    for (int i = 0; i < selection.count(); i++) {
        ui->listWidget_Tab3->setItemSelected(selection[i], false);
    }
}

void MainWindow::on_pushButton_GetRoutingTable_clicked()
{
    send2selection(ui->listWidget_Tab2, "Get routing table");
}

void MainWindow::on_pushButton_SendTemp_clicked()
{
    send2selection(ui->listWidget_Tab3, "Send temperature threshold");
}

void MainWindow::on_pushButton_SendHum_clicked()
{
    send2selection(ui->listWidget_Tab3, "Send humidity threshold");
}

void MainWindow::on_pushButton_SendLight_clicked()
{
    send2selection(ui->listWidget_Tab3, "Send light threshold");
}

void MainWindow::on_pushButton_SendAll_clicked()
{
    send2selection(ui->listWidget_Tab3, "Send all thresholds");
}

void MainWindow::on_pushButton_GetAll_clicked()
{
    send2selection(ui->listWidget_Tab3, "Get all thresholds");
}

void MainWindow::on_pushButton_Center_clicked()
{
    QRectF bounds = mScene->itemsBoundingRect();
    bounds.setWidth(bounds.width()*0.95);         // to tighten-up margins
    bounds.setHeight(bounds.height()*0.95);       // same as above
    ui->graphicsView_Networkgraph->fitInView(bounds,Qt::KeepAspectRatio);
}

void MainWindow::on_pushButton_Clear_clicked()
{
    ui->textEdit_Status->clear();
}

void MainWindow::on_pushButton_LED_clicked()
{
    send2selection(ui->listWidget_Tab3, "Set LED color");
}
