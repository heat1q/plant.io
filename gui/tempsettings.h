#ifndef TEMPSETTINGS_H
#define TEMPSETTINGS_H

#include <QDialog>

namespace Ui {
class TempSettings;
}

class TempSettings : public QDialog
{
    Q_OBJECT

public:
    explicit TempSettings(QWidget *parent = nullptr);
    ~TempSettings();

private:
    Ui::TempSettings *ui;
};

#endif // TEMPSETTINGS_H
