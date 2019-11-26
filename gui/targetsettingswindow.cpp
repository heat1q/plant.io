#include "targetsettingswindow.h"
#include "ui_targetsettingswindow.h"

TargetSettingsWindow::TargetSettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TargetSettingsWindow)
{
    ui->setupUi(this);
}

TargetSettingsWindow::~TargetSettingsWindow()
{
    delete ui;
}
