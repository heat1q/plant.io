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

void MainWindow::receive()
// QObject::connect(&port, SIGNAL(readyRead()), this, SLOT(receive()));
{
    static QString str;
    char ch;
    while (port.getChar(&ch))
    {
        str.append(ch);
        if (ch == '\n')     // End of line, start decoding
        {
            str.remove("\n", Qt::CaseSensitive);
            ui->textEdit_Status->insertPlainText(str);
            this->repaint();    // Update content of window immediately
            str.clear();
        }
    }
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
    double circle_radius=100+4.20*n_max;

    for(int i = 0; i<InputList.size()-1; i++) // Iterate all items in header "11:14:7:215:PAYLOAD"
    {
        int target_id = InputList[i].toInt();

        if (target_id >= n_max){ // doesn't check if header data is valid. invalid = 0. fix!?
            ui->textEdit_Status->insertPlainText("Invalid header id. Abort mission\n");
            break;
        }

        if ((abs(node_pos[target_id][0]) > 0) || (abs(node_pos[target_id][1]) > 0)){ // Target node already exists
            //qDebug() << "Target node" << target_id << "already exists";
        }

        else{ // target node doesn't exist
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
                new_alpha = atan2(curr_pos[1],curr_pos[0]) + (node_pos[curr_id][2]) / double(2.5*i*n_max) * 2*pi;
                x_offset = new_length*cos(new_alpha)-curr_pos[0];
                y_offset = new_length*sin(new_alpha)-curr_pos[1];
            }
            //int cnt = int(node_pos[target_id][2]);
            node_pos[target_id][0] = curr_pos[0] + x_offset;
            node_pos[target_id][1] = curr_pos[1] + y_offset;

            // increment outgoing edge counter
            node_pos[curr_id][2] += 1;

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

    on_pushButton_Center_clicked();
}

void MainWindow::on_pushButton_CreateRoute_clicked()
// test button for graph
{
    QString Input = ui->plainTextEdit_Create->toPlainText();
    QStringList InputList = Input.split(":");
    MainWindow::create_graph(InputList);
}

void MainWindow::send2port(QString Input)
// Send text field message to port
{
    QByteArray byteArray = Input.toLocal8Bit();
    byteArray.append('\n');
    port.write(byteArray);
}

void MainWindow::on_pushButton_Explore_clicked()
{
    // create networkgraph
    mScene = new QGraphicsScene();
    ui->graphicsView_Networkgraph->setScene( mScene );

    // disable notification text
    //ui->label_networkgraph->setVisible(0);

    // create root node
    QBrush redBrush(Qt::red);
    QPen blackPen(Qt::black);
    blackPen.setWidth(2);
    mScene->addEllipse(-10,-10,20,20,blackPen,redBrush);

    // enable node creation button
    ui->pushButton_CreateRoute->setEnabled(true);

    // reset graph
    reset_graph();

    // devide circle angles into n_max equally spaced values
    for (int i = 0; i < n_max; ++i) {
        alpha.insert(i, i / double(n_max) * 2 * pi);
    }
}

void MainWindow::on_pushButton_ZoomIn_clicked()
{
    ui->graphicsView_Networkgraph->scale(1.2,1.2);
}

void MainWindow::on_pushButton_ZoomOut_clicked()
{
    ui->graphicsView_Networkgraph->scale(0.8,0.8);
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

void MainWindow::on_pushButton_SetMax_clicked()
{
    QString Input = ui->plainTextEdit_SetMax->toPlainText();
    if(Input.toInt()<n_limit){
        n_max = Input.toInt();
        on_pushButton_Explore_clicked();
    }
    else{ui->textEdit_Status->insertPlainText("Please choose a value below " + QString::number(n_limit) + ".\n");}
}

void MainWindow::on_pushButton_creategraph_clicked()
{
    QCustomPlot* plot = ui->customPlot;
    plot->plotLayout()->clear();
    plot->clearItems();
    plot->clearGraphs();
    plot->plotLayout()->simplify();

    QVector<double> x(61),y(61);
    for (int i = 0; i < 61; ++i) {
        x[i] = i/50.0 - 1;
        y[i] = 4*x[i]*x[i];
    }

    QVector<double> x2(101),y2(101);
    for (int i = 0; i < 101; ++i) {
        x2[i] = i/50.0 - 1;
        y2[i] = x2[i]*x2[i];
    }

    QVector<double> x3(200),y3(200);
    for (int i = 0; i < 200; ++i) {
        x3[i] = 0.1*i - 10;
        y3[i] = x3[i]*x3[i]*3/i;
    }

    QCPAxisRect *TempAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(0, 0, TempAxisRect);

    QCPAxisRect *HumAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(1, 0, HumAxisRect);

    QCPAxisRect *LightAxisRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(2, 0, LightAxisRect);

    QCPLegend *Legend = new QCPLegend;
    plot->axisRect()->insetLayout()->addElement(Legend, Qt::AlignTop|Qt::AlignRight);

    for (int i = 0; i < 3; ++i) {
        plot->axisRect(i)->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom);
        plot->axisRect(i)->setMargins(QMargins(0,0,100,0));
    }
    plot->axisRect(0)->insetLayout()->setInsetPlacement(0, QCPLayoutInset::ipFree);
    plot->axisRect(0)->insetLayout()->setInsetRect(0, QRectF(1,0,0,0));

    plot->setAutoAddPlottableToLegend(false);

    int listCount = ui->listWidget_Tab2->selectedItems().count();
    QVector<QCPGraph *> TempGraphVector;
    QVector<QCPGraph *> HumGraphVector;
    QVector<QCPGraph *> LightGraphVector;
    QList<QListWidgetItem *> ids = ui->listWidget_Tab2->selectedItems();



    for (int i = 0; i < listCount; i++) {
        // get sensor data from mote id: i
        QString id = (*ids[i]).text().split("ID")[1];
        ui->textEdit_Status->insertPlainText("ID#"+id+"\n");

        QCPGraph *TempGraph = plot->addGraph(TempAxisRect->axis(QCPAxis::atBottom), TempAxisRect->axis(QCPAxis::atLeft));
        QCPGraph *HumGraph = plot->addGraph(HumAxisRect->axis(QCPAxis::atBottom), HumAxisRect->axis(QCPAxis::atLeft));
        QCPGraph *LightGraph = plot->addGraph(LightAxisRect->axis(QCPAxis::atBottom), LightAxisRect->axis(QCPAxis::atLeft));

        // TempGraph->setData(node_data[i][0],node_data[i][1]);

        QVector<double> x(61),y(61);
        for (int j = 0; j < 61; ++j) {
            x[j] = j/50.0 - 1;
            y[j] = i*4*x[j]*x[j];
        }
        TempGraph->setName("ID#"+id);
        TempGraph->setData(x,y);
        TempGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black), QBrush(Qt::white), 3));
        TempGraph->setPen(QPen(QColor((255+i*100)%255, (255+i*200)%255, (255+i*300)%255), 2));
        TempGraph->keyAxis()->setLabel("Time");
        TempGraph->valueAxis()->setLabel("Temp in °C");
        TempGraph->rescaleAxes();
        TempGraphVector.append(TempGraph);
        Legend->addItem(new QCPPlottableLegendItem(Legend, TempGraphVector[i])); //is enough if done for first graph only (shared legend)

        HumGraph->setName("H - ID#"+id);
        HumGraph->setData(x2,y2);
        HumGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black), QBrush(Qt::white), 3));
        HumGraph->setPen(QPen(QColor(120, 120, 120), 2));
        HumGraph->keyAxis()->setLabel("Time");
        HumGraph->valueAxis()->setLabel("Humidity in %");
        HumGraph->rescaleAxes();
        HumGraphVector.append(HumGraph);

        LightGraph->setName("L - ID#"+id);
        LightGraph->setData(x3,y3);
        LightGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::green), QBrush(Qt::red), 3));
        LightGraph->setPen(QPen(QColor(120, 255, 120), 2));
        LightGraph->keyAxis()->setLabel("Time");
        LightGraph->valueAxis()->setLabel("Light in Lumen");
        LightGraph->rescaleAxes();
        LightGraphVector.append(LightGraph);
    }

    QList<QCPAxis*> allAxes;
    allAxes << TempAxisRect->axes() << HumAxisRect->axes() << LightAxisRect->axes();
    foreach (QCPAxis *axis, allAxes)
    {
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");
    }

    plot->replot();
}

void MainWindow::on_pushButton_Refresh_Tab2_clicked()
{
    ui->listWidget_Tab2->clear();
    for (int i = 0; i < n_max; ++i) {
        if ((int(node_pos[i][0])!=0)||(int(node_pos[i][1])!=0)){
            QString message = "Zolertia™ Re-Mote ID" + QString::number(i);
            QListWidgetItem *listItem = new QListWidgetItem(
                        QIcon("/home/andreas/Documents/University/Wireless "
                              "Sensor Networks/plant.io/gui/resource/remote.png"), message, ui->listWidget_Tab2);
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

/* Tab 3 */

void MainWindow::on_pushButton_Refresh_clicked()
{
    ui->listWidget->clear();
    for (int i = 0; i < n_max; ++i) {
        if ((int(node_pos[i][0])!=0)||(int(node_pos[i][1])!=0)){
            QString message = "Zolertia™ Re-Mote ID" + QString::number(i);
            QListWidgetItem *listItem = new QListWidgetItem(
                        QIcon("/home/andreas/Documents/University/Wireless "
                              "Sensor Networks/plant.io/gui/resource/remote.png"), message, ui->listWidget);
            ui->listWidget->addItem(listItem);
        }
    }
}

void MainWindow::on_pushButton_SelectAll_clicked()
{
    ui->listWidget->selectAll();
}

void MainWindow::on_pushButton_UnselectAll_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    for (int i = 0; i < selection.count(); i++) {
        ui->listWidget->setItemSelected(selection[i], false);
    }
}

void MainWindow::on_pushButton_SendTemp_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    if ((selection.count() > 0) && (port.isOpen())){
        double minTemp = ui->lcdNumber_MinTemp->value();
        double maxTemp = ui->lcdNumber_MaxTemp->value();

        for (int i = 0; i < selection.count(); ++i) {
            QListWidgetItem listItem = *selection.at(i);
            QStringList stringList = listItem.text().split("ID");
            QString id = stringList[1];
            QString cmd = id + ":temp:" + QString::number(minTemp) + ":" + QString::number(maxTemp);
            QByteArray byteArray = cmd.toLocal8Bit();
            byteArray.append('\n');
            port.write(byteArray);
            qDebug() << "port.write" << byteArray;
        }
    }
    else {
        ui->textEdit_Status->insertPlainText("No target motes selected OR no port connection!\n");
    }
}

void MainWindow::on_pushButton_SendHum_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    if ((selection.count() > 0) && (port.isOpen())){
        double minHum = ui->lcdNumber_MinHum->value();
        double maxHum = ui->lcdNumber_MaxHum->value();

        for (int i = 0; i < selection.count(); ++i) {
            QListWidgetItem listItem = *selection.at(i);
            QStringList stringList = listItem.text().split("ID");
            QString id = stringList[1];
            QString cmd = id + ":hum:" + QString::number(minHum) + ":" + QString::number(maxHum);
            QByteArray byteArray = cmd.toLocal8Bit();
            byteArray.append('\n');
            port.write(byteArray);
            qDebug() << "port.write" << byteArray;
        }
    }
    else {
        ui->textEdit_Status->insertPlainText("No target motes selected OR no port connection!\n");
    }
}

void MainWindow::on_pushButton_SendLight_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    if ((selection.count() > 0) && (port.isOpen())){
        double minLight = ui->lcdNumber_MinLight->value();
        double maxLight = ui->lcdNumber_MaxLight->value();

        for (int i = 0; i < selection.count(); ++i) {
            QListWidgetItem listItem = *selection.at(i);
            QStringList stringList = listItem.text().split("ID");
            QString id = stringList[1];
            QString cmd = id + ":light:" + QString::number(minLight) + ":" + QString::number(maxLight);
            QByteArray byteArray = cmd.toLocal8Bit();
            byteArray.append('\n');
            port.write(byteArray);
            qDebug() << "port.write" << byteArray;
        }
    }
    else {
        ui->textEdit_Status->insertPlainText("No target motes selected OR no port connection!\n");
    }
}

void MainWindow::on_pushButton_SendAll_clicked()
{
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    if ((selection.count() > 0) && (port.isOpen())){
        double minTemp = ui->lcdNumber_MinTemp->value();
        double maxTemp = ui->lcdNumber_MaxTemp->value();
        double minHum = ui->lcdNumber_MinHum->value();
        double maxHum = ui->lcdNumber_MaxHum->value();
        double minLight = ui->lcdNumber_MinLight->value();
        double maxLight = ui->lcdNumber_MaxLight->value();

        for (int i = 0; i < selection.count(); ++i) {
            QListWidgetItem listItem = *selection.at(i);
            QStringList stringList = listItem.text().split("ID");
            QString id = stringList[1];
            QString cmd = id + ":all:" + QString::number(minTemp) + ":" + QString::number(maxTemp)
                    + ":" + QString::number(minHum) + ":" + QString::number(maxHum)
                    + ":" + QString::number(minLight) + ":" + QString::number(maxLight);
            QByteArray byteArray = cmd.toLocal8Bit();
            byteArray.append('\n');
            port.write(byteArray);
            qDebug() << "port.write" << byteArray;
        }
    }
    else {
        ui->textEdit_Status->insertPlainText("No target motes selected OR no port connection!\n");
    }
}

// Set threshold values
void MainWindow::on_pushButton_SetTemp_clicked()
{
    double minTemp = ui->doubleSpinBox_MinTemp->value();
    ui->lcdNumber_MinTemp->display(minTemp);
    double maxTemp = ui->doubleSpinBox_MaxTemp->value();
    ui->lcdNumber_MaxTemp->display(maxTemp);
}

void MainWindow::on_pushButton_SetHum_clicked()
{
    double minHum = ui->doubleSpinBox_MinHum->value();
    ui->lcdNumber_MinHum->display(minHum);
    double maxHum = ui->doubleSpinBox_MaxHum->value();
    ui->lcdNumber_MaxHum->display(maxHum);
}

void MainWindow::on_pushButton_SetLight_clicked()
{
    double minLight = ui->doubleSpinBox_MinLight->value();
    ui->lcdNumber_MinLight->display(minLight);
    double maxLight = ui->doubleSpinBox_MaxLight->value();
    ui->lcdNumber_MaxLight->display(maxLight);
}

void MainWindow::on_pushButton_SetAll_clicked()
{
    on_pushButton_SetTemp_clicked();
    on_pushButton_SetHum_clicked();
    on_pushButton_SetLight_clicked();
}

// Debug window
void MainWindow::on_pushButton_Debug_clicked()
{
    QString command;
    command = ui->plainTextEdit_Debug->toPlainText();
    QByteArray byteArray = command.toLocal8Bit();
    byteArray.append('\n');
    port.write(byteArray);
}

void MainWindow::on_pushButton_Center_clicked()
{
    QRectF bounds = mScene->itemsBoundingRect();
    //bounds.setWidth(bounds.width()*0.9);         // to tighten-up margins
    //bounds.setHeight(bounds.height()*0.9);       // same as above
    ui->graphicsView_Networkgraph->fitInView(bounds,Qt::KeepAspectRatio);
}
