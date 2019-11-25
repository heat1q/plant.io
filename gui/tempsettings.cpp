#include "tempsettings.h"
#include "ui_tempsettings.h"

TempSettings::TempSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TempSettings)
{
    ui->setupUi(this);
}

TempSettings::~TempSettings()
{
    delete ui;
}
