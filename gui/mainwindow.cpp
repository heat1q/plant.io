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

void MainWindow::create_graph(QStringList InputList) // Add a route to the graph
{
    // Paint edges and nodes
    QBrush greenBrush(Qt::green);
    QPen blackPen(Qt::black);
    blackPen.setWidth(1);

    curr_pos[0] = 0; // Initial x position
    curr_pos[1] = 0; // Initial y position
    static int curr_id = 0;

    double x,y,x_offset,y_offset,len,new_length,new_alpha;
    double circle_radius=100+4.20*n_max;

    for(int i = InputList.size()-1; i>=0; i--) // Iterate all items in header "11:14:7:215"
    {
        int target_id = InputList[i].toInt();

        if (target_id >= n_max){ // doesn't check if header data is valid. invalid = 0. fix!?
            print("Invalid header id. Abort mission\n");
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
                new_length = len + circle_radius;
                new_alpha = atan2(curr_pos[1],curr_pos[0]) + (node_pos[curr_id][2]) / double(2.5*(i+1)*n_max) * 2*pi;
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

void MainWindow::reset_graph() // Reset the graph
{
    // reset all values in node_pos
    for (int row = 0; row < n_max; ++row) // step through the rows in the array
        for (int col = 0; col < 3; ++col) // step through each element in the row
            node_pos[row][col] = 0;

    // reset alpha
    alpha.clear();
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

// outside function for plot() to access the graphs
static QCPAxisRect *TempAxisRect;
static QCPAxisRect *HumAxisRect;
static QCPAxisRect *LightAxisRect;
static QCPLegend *legend;
static QVector<int> existingLegendIDs;

void MainWindow::plot(int type, QStringList data) // ID : SENSOR_DATA : TYPE : DATA
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
    QString id = data[0]; // ID from which the data came from
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
        //qDebug() << ch;
        str.append(ch);
        //msg.append(ch);
        if (ch == '<'){
            str.clear();
        }
        else if (ch == '>') {
            str.remove(">");
            print(str+"\n");
            /* INC DATA STRUCTURE:
             * <$src_id:route:$r_1:$_2:...>
             * # get optimal path to node with id $src_id
             * <$src_id:th:$temp_low:$temp_high:$hum_low:$hum_high:$light_low:$light_high>
             * # get thresholds of node with id $src_id
             * <$src_id:sensors_data:$i_1:...:$i_max>
             * # get sensors data of node with id $src_id with datapoint $i_1 to $i_max
             * <$src_id:rt:$data>
             * # get routing table of node with id $src_id with $data formatted equivalent to file structure
             */

            QStringList data = str.split(":");
            if (data[1] == "route"){
                data.removeFirst();
                data.removeFirst();
                create_graph(data); // create visualization of route which also registers the ID's as valid targets
            }
            else if (data[1] == "sensor_data"){ // ID : SENSOR_DATA : TYPE : DATA
                int type = data[2].toInt();
                data.removeAt(2);
                plot(type, data);
            }
            else if (data[1] == "th") {
                print("pls fix me :(");
            }
            else if (data[1] == "rt") {
                print("pls fix me :(");
            }

            str.clear();
        }
        else if (ch == '\n')     // End of line, start decoding
        {
            //str.remove("\n", Qt::CaseSensitive);
            print(str);
            /*
            QStringList split = str.split(":"); // <1:route:1:2:3:4:...>
            if (split[1] == "route"){

            }
            if (str.startsWith(""))
            //create_graph(str)
            */
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

void MainWindow::on_pushButton_CreateRoute_clicked() // test button for graph
{
    QString Input = ui->plainTextEdit_Create->toPlainText();
    QStringList InputList = Input.split(":");
    MainWindow::create_graph(InputList);
}

void MainWindow::on_pushButton_Explore_clicked()
{
    send2port("0:init:"); // Initialize network exploration on mote

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

    ui->pushButton_Explore->setEnabled(false);
    QTimer::singleShot(3000, this, SLOT(enableButton())); // first entry is time in ms
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
    QString Input = ui->plainTextEdit_SetMax->toPlainText();
    if(Input.toInt()<n_limit){
        n_max = Input.toInt();
        on_pushButton_Explore_clicked();
    }
    else{print("Please choose a value below " + QString::number(n_limit) + ".\n");}
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

    // Fetch mote ID selection from listWidget
    int listCount = ui->listWidget_Tab2->selectedItems().count();
    QList<QListWidgetItem *> ids = ui->listWidget_Tab2->selectedItems();

    // Iterate through all list items and request the data @GUI mote
    for (int i = 0; i < listCount; i++) {
        // get sensor data from mote id: i
        QString id;
        if ((*ids[i]).text().contains("GUI")){
            id = "0"; // GUI TARGET ID
        }
        else{
            id = (*ids[i]).text().split("ID")[1];
        }

        send2port(id + ":get_data:1");
        send2port(id + ":get_data:2");
        send2port(id + ":get_data:3");
    }

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

void MainWindow::on_pushButton_GetRoutingTable_clicked()
{
    // Fetch mote ID selection from listWidget
    int listCount = ui->listWidget_Tab2->selectedItems().count();
    QList<QListWidgetItem *> ids = ui->listWidget_Tab2->selectedItems();

    // Iterate through all list items and request the data @GUI mote
    for (int i = 0; i < listCount; i++) {
        // get sensor data from mote id: i
        QString id;
        if ((*ids[i]).text().contains("GUI")){
            id = "0"; // GUI TARGET ID
        }
        else{
            id = (*ids[i]).text().split("ID")[1];
        }

        send2port(id + ":rt");
    }
}

void MainWindow::on_pushButton_Refresh_Tab2_clicked()
{
    ui->listWidget_Tab2->clear();
    QString message = "Zolertia™ GUI Re-Mote";
    QListWidgetItem *listItem = new QListWidgetItem(
                QIcon("/home/andreas/Documents/University/Wireless "
                      "Sensor Networks/plant.io/gui/resource/remote.png"), message, ui->listWidget_Tab2);
    ui->listWidget_Tab2->addItem(listItem); // ADD GUI MOTE OPTION MANUALLY

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

void MainWindow::on_pushButton_Refresh_clicked() // Tab 3
{
    ui->listWidget->clear();
    QString message = "Zolertia™ GUI Re-Mote";
    QListWidgetItem *listItem = new QListWidgetItem(
                QIcon("/home/andreas/Documents/University/Wireless "
                      "Sensor Networks/plant.io/gui/resource/remote.png"), message, ui->listWidget_Tab2);
    ui->listWidget_Tab2->addItem(listItem); // ADD GUI MOTE OPTION MANUALLY

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

// Send threshold values
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
            QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp) + ":-1:-1:-1:-1";
            send2port(cmd);
        }
    }
    else {
        print("No target motes selected OR no port connection!\n");
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
            QString cmd = id + ":set_th:-1:-1:" + QString::number(minHum) + ":" + QString::number(maxHum) + ":-1:-1";
            send2port(cmd);
        }
    }
    else {
        print("No target motes selected OR no port connection!\n");
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
            QString cmd = id + ":set_th:-1:-1:-1:-1:" + QString::number(minLight) + ":" + QString::number(maxLight);
            send2port(cmd);
        }
    }
    else {
        print("No target motes selected OR no port connection!\n");
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
            QString cmd = id + ":set_th:" + QString::number(minTemp) + ":" + QString::number(maxTemp)
                    + ":" + QString::number(minHum) + ":" + QString::number(maxHum)
                    + ":" + QString::number(minLight) + ":" + QString::number(maxLight);
            send2port(cmd);
        }
    }
    else {
        print("No target motes selected OR no port connection!\n");
    }
}

void MainWindow::on_pushButton_Debug_clicked()
{
    QString cmd = ui->plainTextEdit_Debug->toPlainText();
    send2port(cmd);
}

void MainWindow::on_pushButton_Center_clicked()
{
    QRectF bounds = mScene->itemsBoundingRect();
    //bounds.setWidth(bounds.width()*0.9);         // to tighten-up margins
    //bounds.setHeight(bounds.height()*0.9);       // same as above
    ui->graphicsView_Networkgraph->fitInView(bounds,Qt::KeepAspectRatio);
}

void MainWindow::on_pushButton_Clear_clicked()
{
    ui->textEdit_Status->clear();
}
