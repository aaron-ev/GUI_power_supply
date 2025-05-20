/**
 * @file GUI_MAIN_POWER_SUPPLY.cpp
 * @brief Main implementation file for the GUI Power Supply application.
 *
 * This file contains the implementation of the MainWindow class and the Worker class,
 * which together provide the main logic for the GUI-based power supply controller.
 * The application allows users to control and monitor a power supply device via a VISA-based serial interface.
 *
 * Features:
 * - Open/close serial port
 * - Set and monitor voltage/current
 * - Toggle power supply output
 * - Pin/unpin the main window (always on top)
 * - Save and restore user settings
 * - Threaded worker for background current monitoring
 *
 * @author Aaron Escoboza
 * @note Github: https://github.com/aaron-ev
 */

#include "GUI_MAIN_POWER_SUPPLY.h"
#include "./ui_UI_POWER_SUPPLY.h"
#include <QObject>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>

/**
 * @class Worker
 * @brief Background worker for monitoring power supply current in a separate thread.
 *
 * The Worker class periodically queries the power supply for the current value and emits
 * a signal when the current changes. It is designed to run in a separate thread.
 */
class Worker : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param parent Parent QObject.
     * @param ps Pointer to the PowerSupply object.
     */
    explicit Worker(QObject *parent = nullptr, PowerSupply *ps = nullptr) : QObject(parent), powerSupply(ps){}

    /**
     * @brief Requests the worker to stop.
     */
    void stop(void)
    {
        stopFlag = true;
    }

private:
    PowerSupply *powerSupply;      ///< Pointer to the PowerSupply object.
    PowerSupply::PsError err;      ///< Last error code.
    double oldCurrent = 0.0;       ///< Previous current value.
    double newCurrent = 0.0;       ///< Latest current value.
    bool stopFlag = false;         ///< Flag to stop the worker loop.
    int sampleTime = 1;            ///< Time between samples in seconds.

signals:
    /**
     * @brief Signal emitted when the current value changes.
     * @param current The new current value.
     */
    void currentChanged(double current);

public slots:
    /**
     * @brief Main worker loop. Periodically queries the power supply for current.
     */
    void mainWork()
    {
        while (stopFlag == false)
        {
            if (powerSupply->isOpen() != PowerSupply::PsError::ERR_SUCCESS)
            {
                qDebug() << "Port not open";
                goto wait_till_nex_sample;
            }

            err = powerSupply->readCurrent(newCurrent);
            if (err != PowerSupply::PsError::ERR_SUCCESS)
            {
                qDebug() << "Failed to get current";
                goto wait_till_nex_sample;
            }

            /* Only signal is emitted when there is a current change */
            if (newCurrent != oldCurrent)
            {
                oldCurrent = newCurrent;
                emit currentChanged(newCurrent);
            }

            wait_till_nex_sample:
                QThread::sleep(sampleTime); /* Wait until next sample */
        }
    }
};

/**
 * @class MainWindow
 * @brief Main application window for the GUI Power Supply.
 *
 * Handles user interaction, UI updates, and manages the worker thread for background monitoring.
 */

/**
 * @brief MainWindow constructor.
 * Initializes the UI, restores user settings, and starts the worker thread.
 * @param parent Parent widget.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QString userPort;
    bool powerState = false;
    bool userPinState = false;
    PowerSupply::PsError err = PowerSupply::PsError::ERR_SUCCESS;

    ui->setupUi(this);
    setFixedSize(size());
    this->setWindowTitle(this->windowTitle() + " v" + swVersion);

    /* User settings: Port */
    settings = new QSettings("powerSupply", "settings");

    /* User settings: Port */
    userPort = settings->value("port", "").toString();
    if (userPort.isEmpty())
        ui->port->setText("COM");
    else
        ui->port->setText(userPort);

    /* User settings: Pin state */
    userPinState = settings->value("pinState", false).toBool();
    if (userPinState)
    {
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        ui->pinButton->setChecked(userPinState);
    }

    /* Power supply object */
    powerSupply = new PowerSupply(userPort.toStdString());

    /* Create worker thread, connect signals and start it */
    workerThread = new QThread(this);
    worker = new Worker(nullptr, powerSupply);
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &Worker::mainWork);
    connect(workerThread, &QThread::finished, worker, &Worker::deleteLater);
    connect(worker, &Worker::currentChanged, this, &MainWindow::on_current_valueChanged);

    /* Check if power supply port is opened */
    if (powerSupply->isOpen() != PowerSupply::PsError::ERR_SUCCESS)
        QString errorMessage = "Failed to turn on power supply";

    /* Update power button state */
    powerSupply->isOn(powerState);
    if (powerState)
        load_power_icon(ui->buttonPower, true);
    else
        load_power_icon(ui->buttonPower, false);

    /* User settings: default voltage */
    lastSavedVoltage = settings->value("lastSavedVoltage", "0.0").toDouble();
    if (powerSupply->writeVoltage(lastSavedVoltage) == PowerSupply::PsError::ERR_SUCCESS)
    {
        ui->voltage->blockSignals(true);
        ui->voltage->setValue(lastSavedVoltage);
        ui->voltage->blockSignals(false);
    }
    workerThread->start();
}

/**
 * @brief Handles the window close event.
 * Stops the worker thread and closes the power supply session.
 * @param event Pointer to the close event.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    /* Close the power supply */
    if (powerSupply)
    {
        powerSupply->close();
    }

    /* Stop the worker thread */
    if (workerThread)
    {
        worker->stop();
        workerThread->quit();
        workerThread->wait();
        delete workerThread;
    }

    event->accept();  // Accept the close event
}

/**
 * @brief MainWindow destructor.
 * Cleans up resources and stops the worker thread.
 */
MainWindow::~MainWindow()
{
    if (powerSupply)
    {
        powerSupply->close();
        delete powerSupply;
    }

    if (workerThread)
    {
        workerThread->quit();          // Request the thread to quit
        workerThread->wait();          // Wait for the thread to finish
        delete workerThread;           // Delete the thread
    }
    delete ui;  // Clean up the UI
}

/**
 * @brief Slot called when the current value changes.
 * @param current The new current value.
 */
void MainWindow::on_current_valueChanged(double current)
{
    ui->current->setValue(current);
}

/**
 * @brief Slot called when the voltage value changes.
 * @param voltage The new voltage value.
 */
void MainWindow::on_voltage_valueChanged(double voltage)
{
   if (powerSupply->writeVoltage(voltage) != PowerSupply::PsError:: ERR_SUCCESS)
   {
       ui->voltage->setValue(0.0);
       return;
   }
   lastSavedVoltage = voltage;
   settings->setValue("lastSavedVoltage", lastSavedVoltage);
}

/**
 * @brief Resets the power supply widgets to their default state.
 */
void MainWindow::reset_power_supply_widgets(void)
{
    /* Reset widgets to default values: Power supply off */
    load_power_icon(ui->buttonPower, false);
    ui->voltage->blockSignals(true);
    ui->voltage->setValue(0.0);
    ui->voltage->blockSignals(false);
    ui->current->setValue(0.0);
}

/**
 * @brief Slot called when the power button is clicked.
 * Turns the power supply on or off.
 */
void MainWindow::on_buttonPower_clicked()
{
    double voltage;
    double current;
    bool powerState;
    PowerSupply::PsError err;
    QString errorMessage = "";

    /* Check if the device is opened */
    if (powerSupply->isOpen() != PowerSupply::PsError::ERR_SUCCESS)
    {
        errorMessage = QString("Port %1 not open").arg(powerSupply->port);
        goto err_buttonPower_clicked;
    }

    lastSavedVoltage = settings->value("lastSavedVoltage", "0.0").toDouble();

    /* Check if if the device is turned ON */
    err = powerSupply->isOn(powerState);
    if (err != PowerSupply::PsError::ERR_SUCCESS)
    {
        errorMessage = "Failed to get power supply state";
        goto err_buttonPower_clicked;
    }

    if (powerState == true) /* Power state ON, next state OFF */
    {
        err = powerSupply->turnOff();
        if (err != PowerSupply::PsError::ERR_SUCCESS)
        {
            errorMessage = "Failed to turn off device";
            goto err_buttonPower_clicked;
        }
        reset_power_supply_widgets();
    }
    else /* Power state OFF, next state ON */
    {
        err = powerSupply->turnOn();
        if (err != PowerSupply::PsError::ERR_SUCCESS)
        {
            errorMessage = "Failed to turn on device";
            goto err_buttonPower_clicked;
        }

        /* Power supply is on, voltage updated to user default values */
        load_power_icon(ui->buttonPower, true);
        if (powerSupply->writeVoltage(lastSavedVoltage) == PowerSupply::PsError::ERR_SUCCESS)
        {
            ui->current->setValue(0.0);
            ui->voltage->blockSignals(true);
            ui->voltage->setValue(lastSavedVoltage);
            ui->voltage->blockSignals(false);
        }
    }

    return;

err_buttonPower_clicked:
    QMessageBox::critical(this, "Error", errorMessage);
}

/**
 * @brief Loads the appropriate power icon for the button based on state.
 * @param button Pointer to the QPushButton.
 * @param state True if power is on, false otherwise.
 */
void MainWindow:: load_power_icon(QPushButton *button, bool state)
{
    /* Load the right image according to the current power state */
    if (state)
    {
        QPixmap pixMapPsOnButton(powerSwitchOnStatePath.c_str());
        if (!pixMapPsOnButton.isNull())
        {
            button->setIcon(QIcon(pixMapPsOnButton));
            button->setIconSize(QSize(powerSwitchSize, powerSwitchSize));
        }
    }
    else
    {
        QPixmap pixMapPsOffButton(powerSwitchOffStatePath.c_str());
        if (!pixMapPsOffButton.isNull())
        {
            button->setIcon(QIcon(pixMapPsOffButton));
            button->setIconSize(QSize(powerSwitchSize, powerSwitchSize));
        }
    }
}

/**
 * @brief Slot called when the pin button is clicked.
 * Pins or unpins the main window (always on top).
 * @param checked True if the button is checked (pinned).
 */
void MainWindow::on_pinButton_clicked(bool checked)
{
    /* Window flags are updated to put the window always on
       top. Otherwise, the window is set to normal state.
       The window is shown again to apply the new flags.
    */
    if (checked)
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
    else
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnTopHint);
    settings->setValue("pinState", checked);
    this->show();
}

void MainWindow::on_voltage_editingFinished()
{
    double voltage = ui->voltage->value();
    if (voltage < 0.0)
    {
        QMessageBox::warning(this, "Invalid Voltage", "Voltage must be greater than 0.0V");
        return;
    }
    /* Save to user settings */
    lastSavedVoltage = voltage;
    settings->setValue("lastSavedVoltage", lastSavedVoltage);
}

void MainWindow::on_port_editingFinished()
{
    QString port;
    double voltage;
    bool powerState;
    PowerSupply::PsError err;
    QString errorMessage = "";

    /* Check for empty string */
    port = ui->port->text();
    if (port.isEmpty())
    {
        errorMessage = "Empty port";
        goto err_open_port;
    }

    /* Check if port is already open */
    if (powerSupply->port == port.toStdString() && powerSupply->isOpen() == PowerSupply::PsError::ERR_SUCCESS)
        return;

    /* Attempt to open the serial port */
    if (powerSupply->open(port.toStdString()) != PowerSupply::PsError::ERR_SUCCESS)
    {
        reset_power_supply_widgets();
        errorMessage = "Failed to open port " + port;
        goto err_open_port;
    }

    /* Save opened port to user settings */
    settings->setValue("port", port);

    /* Check the current power state */
    err = powerSupply->isOn(powerState);
    if (err != PowerSupply::PsError::ERR_SUCCESS)
    {
        errorMessage = "Failed to get power supply state";
        goto err_open_port;
    }

    /* Update widgets with current power state */
    if (powerState == true)
    {
        load_power_icon(ui->buttonPower, true);
        err = powerSupply->readVoltage(voltage);
        if (err != PowerSupply::PsError::ERR_SUCCESS)
        {
            errorMessage = "Failed to get voltage";
            goto err_open_port;
        }
        /* Signals are blocked to prevent seting the voltage okn the device */
        ui->voltage->blockSignals(true);
        ui->voltage->setValue(voltage);
        ui->voltage->blockSignals(false);
    }
    else
    {
        /* Current power device state is OFF, all widgets are reset */
        reset_power_supply_widgets();
    }

    QMessageBox::information(this, "Success", QString("Port %1 opened successfully").arg(port));
    return;

err_open_port:
    QMessageBox::critical(this, "Error", errorMessage);
    powerSupply->close();
}

#include "GUI_MAIN_POWER_SUPPLY.moc"  // Include the MOC-generated file
