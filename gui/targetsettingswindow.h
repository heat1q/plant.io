#ifndef TARGETSETTINGSWINDOW_H
#define TARGETSETTINGSWINDOW_H

#include <QDialog>

namespace Ui {
class TargetSettingsWindow;
}

class TargetSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TargetSettingsWindow(QWidget *parent = nullptr);
    ~TargetSettingsWindow();

private:
    Ui::TargetSettingsWindow *ui;
};

#endif // TARGETSETTINGSWINDOW_H
