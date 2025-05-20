#include <iostream>
#include <cstdint>
#include <cstring>
#include "visa.h"
#include <map>
#include <string>

class PowerSupply
{
    public:
        enum class PsError
        {
            ERR_SUCCESS = 0,
            ERR_INVALID_VOLTAGE,
            ERR_INVALID_CURRENT,
            ERR_DEVICE_NOT_CONNECTED,
            ERR_OPERATION_FAILED
        };

        PowerSupply(std::string port);
        ~PowerSupply();

        PsError open(std::string port);
        PsError writeVoltage(double voltage);
        PsError writeMaxCurrent(double current);
        PsError isOpen(void);
        PsError isOn(bool& state);
        PsError turnOn(void);
        PsError turnOff(void);
        PsError readVoltage(double& voltage);
        PsError readCurrent(double& current);
        void close(void);
        std::string port;
        int baudrate;

    private:
        int defaultBaudrate = 9600;
        ViSession defaultRM;
        ViSession instrument;
        std::map<std::string, std::string> psCommands =
        {
            {"writeVoltage",      "VOLT"},
            {"setCurrent",      "CURR"},
            {"writeMaxCurrent",   "IMAX"},
            {"readVoltage",      "MEAS:VOLT?"},
            {"readCurrent",      "MEAS:CURR?"},
            {"getMaxCurrent",   "IMAX?"},
            {"isOn",            "OUTP?"},
            {"turnOn",          "OUTP ON"},
            {"turnOff",         "OUTP OFF"}
        };
        PsError sendCommand(const std::string& command, const std::string& value);
};
