#ifndef GUI_MAIN_POWER_SUPPLY_H
#define GUI_MAIN_POWER_SUPPLY_H

#include <QMainWindow>
#include "drv_power_supply.h"
#include <QPushButton>
#include <QThread>
#include <QCloseEvent>
#include <mutex>
#include <QSettings>

class Worker;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_buttonPower_clicked();
    void on_pinButton_clicked(bool checked);
    void on_voltage_valueChanged(double voltage);
    void on_current_valueChanged(double current);
    void on_voltage_editingFinished();
    void on_port_editingFinished();

signals:
    void powerSupplyStateChanged(bool state);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Worker *worker;  /* Pointer to the worker object */
    QSettings *settings;  /* Pointer to the QSettings object */
    int powerSwitchSize = 65; /* Default power switch icon size (w, h) */
    Ui::MainWindow *ui;  /* Declare the `ui` member */
    QThread *workerThread;  /* Pointer to the worker thread */
    PowerSupply *powerSupply;  /* Pointer to the PowerSupply object */
    double lastSavedVoltage = 0.0;
    int statusbarMessageTimeout = 5000; /* Default timeout for status bar messages */
    std::string powerSwitchOnStatePath = ":/img/on.png";
    std::string powerSwitchOffStatePath = ":/img/off.png";
    QString swVersion = "1.0"; /* Software version */

    /* Private functions */
    void load_power_icon(QPushButton *button, bool state);
    void reset_power_supply_widgets(void);
    void close(void);
};
#endif /* GUI_MAIN_POWER_SUPPLY_H */
