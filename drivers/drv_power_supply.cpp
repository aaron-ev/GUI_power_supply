
#include "drv_power_supply.h"

/* Define a type alias for key:value pairs */
PowerSupply::PowerSupply(std::string port) 
{
    if (port.empty() || port.size() < 4) 
    {
        std::cout << "Power Supply: Invalid port " << std::endl;
        return;
    }

    /* Update new serial session attributes */
    this->baudrate = defaultBaudrate;
    this->port = port;
    if(open(this->port) != PsError::ERR_SUCCESS) 
        std::cout << "Power Supply: Failed to open port " << this->port << std::endl;
}

PowerSupply::~PowerSupply()
{
    close();
}

PowerSupply::PsError PowerSupply::open(std::string port)
{
    char resourceName[14];
    std::string resourceNameStr;
    ViStatus status = VI_SUCCESS;
    PsError err = PsError::ERR_DEVICE_NOT_CONNECTED;

    memset(resourceName, '\0', sizeof(resourceName));

    /* Check for emtpy port */
    if (port.empty() || port.size() < 4) 
    {
        std::cout << "Power Supply: Invalid port " << std::endl;
        goto err_open;
    }

    /* Open resource manager */
    std::cout << "Power Supply: Opening " << resourceName << std::endl;
    if (viOpenDefaultRM(&this->defaultRM) != VI_SUCCESS) 
    {
        std::cout << "Power Supply: Failed to open default resource manager" << std::endl;
        goto err_open;
    }

    /* Open resource */
    resourceNameStr = ("ASRL" + port.substr(3) + "::INSTR");
    strncpy(resourceName, resourceNameStr.c_str(), sizeof(resourceName));
    if (viOpen(defaultRM, (ViRsrc)resourceName, VI_NULL, VI_NULL, &this->instrument) != VI_SUCCESS) 
    {
        std::cout << "Power Supply: Failed to open instrument" << std::endl;
        goto err_open;
    }

    /* Set instrument configuration:
         - 9600 baud rate
         - 8 data bits
         - No parity
         - 1 stop bit
         - No flow control
         - Termination character: LF (0x0A)
         - Termination character enabled
         - Timeout: 2000 ms 
    */
    viSetAttribute(instrument, VI_ATTR_ASRL_BAUD, this->baudrate);          /* 9600 baud rate */
    viSetAttribute(instrument, VI_ATTR_ASRL_DATA_BITS, 8);                  /* 8 data bits */
    viSetAttribute(instrument, VI_ATTR_ASRL_PARITY, VI_ASRL_PAR_NONE);      /* No parity */
    viSetAttribute(instrument, VI_ATTR_ASRL_STOP_BITS, VI_ASRL_STOP_ONE);   /* 1 stop bit */
    viSetAttribute(instrument, VI_ATTR_ASRL_FLOW_CNTRL, VI_ASRL_FLOW_NONE); /* No flow control */
    viSetAttribute(instrument, VI_ATTR_TERMCHAR, '\n');
    viSetAttribute(instrument, VI_ATTR_TERMCHAR_EN, VI_TRUE);
    viSetAttribute(instrument, VI_ATTR_TMO_VALUE, 2000);                    /* in milliseconds */
    std::cout << "Power Supply: opened resource: \n" << resourceNameStr << std::endl;
    this->port = port;

    /* Port opened successfully */
    return PsError::ERR_SUCCESS;

err_open:
    close();
    return err;
}

PowerSupply::PsError PowerSupply::isOpen(void)
{
    if (instrument == VI_NULL)
        return PsError::ERR_DEVICE_NOT_CONNECTED;
    return PsError::ERR_SUCCESS;
}

PowerSupply:: PsError PowerSupply::isOn(bool& state)
{
    char buffer[50];
    ViUInt32 bufferCount = 0;
    ViStatus status = VI_SUCCESS;
    PsError err = PsError::ERR_SUCCESS;

    state = false;
    memset(buffer, '\0', sizeof(buffer));

    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        err = PsError::ERR_DEVICE_NOT_CONNECTED;
        goto err_isOn;
    }

    /* Send get status command */
    err = sendCommand(psCommands["isOn"], "");
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to get power supply status. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto err_isOn;
    }

    /* Read response from power supply */
    status = viRead(instrument, (unsigned char*)buffer, sizeof(buffer), &bufferCount);
    if (status != VI_SUCCESS)
    {
        std::cout << "Failed to read power supply status. Status: " << status << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto err_isOn;
    }

    /* Parse response to determine if power supply is on */
    if (buffer[0] == '1')
    {
        state = true;
        std::cout << "Power Supply: Device is ON" << std::endl;
    }
    else if (buffer[0] == '0')
    {
        state = false;
        std::cout << "Power Supply: Device is OFF" << std::endl;
    }
    else
    {
        std::cout << "Power Supply: Unknown status response: " << buffer << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
    }

err_isOn:
    return err;
}

PowerSupply::PsError PowerSupply::sendCommand(const std::string& command, const std::string& value)
{
    ViUInt32 commandSize;
    char commandBuffer[30];
    ViStatus status = VI_SUCCESS;
    PsError err = PsError::ERR_SUCCESS;

    memset(commandBuffer, '\0', sizeof(commandBuffer));

    /* Check if command is to be sent with/without parameters */
    if (value.empty())
        /* Command without parameters */
        snprintf(commandBuffer, sizeof(commandBuffer), "%s\n", command.c_str());
    else
        /* Command with parameters */
        snprintf(commandBuffer, sizeof(commandBuffer), "%s %s\n", command.c_str(), value.c_str());

    /* Send command to power supply device */
    std::cout << "Power Supply: Sending command: " << commandBuffer << " (size: " << strlen(commandBuffer) << ")" << std::endl;
    status = viWrite(this->instrument, (unsigned char*)commandBuffer, strlen(commandBuffer), VI_NULL);
    if (status != VI_SUCCESS)
    {
        std::cout << "Failed to send command: status: " << status << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
    }

err_send_command:
    return err;
}

PowerSupply::PsError PowerSupply::setVoltage(double voltage)
{
    PsError err = PsError::ERR_SUCCESS;

    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        return PsError::ERR_DEVICE_NOT_CONNECTED;
    }

    /* Send set voltage command */
    err = sendCommand(psCommands["setVoltage"], std::to_string(voltage));
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to set voltage " << static_cast<int>(voltage) << "V. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
    }
    else
    {
        std::cout << "Power Supply: Set voltage to " << static_cast<int>(voltage) << "V" << std::endl;
    }

ps_err_setVoltage:
    return err;
}

PowerSupply::PsError PowerSupply::getVoltage(double& voltage)
{
    char buffer[25];
    PsError err = PsError::ERR_SUCCESS;
    ViUInt32 bufferCount = 0;
    ViStatus status = VI_SUCCESS;

    memset(buffer, '\0', sizeof(buffer));

    voltage = 0;
    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        return PsError::ERR_DEVICE_NOT_CONNECTED;
    }

    /* Send get voltage command */
    err = sendCommand(psCommands["getVoltage"], "");
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to get voltage. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto ps_err_getVoltage;
    }

    /* Read response from power supply */
    status = viRead(instrument, (unsigned char*)buffer, sizeof(buffer), &bufferCount);
    if (status != VI_SUCCESS)
    {
        std::cout << "Failed to read voltage. Status: " << status << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto ps_err_getVoltage;
    }

    /* Convert response to double */
    voltage = atof(buffer);
    std::cout << "Power Supply: Voltage is " << voltage << "V" << std::endl;

ps_err_getVoltage:
    return err;
}

PowerSupply::PsError PowerSupply::getCurrent(double& current)
{
    char buffer[25];
    ViUInt32 bufferCount = 0;
    ViStatus status = VI_SUCCESS;
    PsError err = PsError::ERR_SUCCESS;

    memset(buffer, '\0', sizeof(buffer));

    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        return PsError::ERR_DEVICE_NOT_CONNECTED;
    }

    /* Send get current command */
    err = sendCommand(psCommands["getCurrent"], "");
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to get current. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto ps_err_getCurrent;
    }

    /* Read response from power supply */
    status = viRead(instrument, (unsigned char*)buffer, sizeof(buffer), &bufferCount);
    if (status != VI_SUCCESS)
    {
        std::cout << "Failed to read current. Status: " << status << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
        goto ps_err_getCurrent;
    }

    /* Convert response to double */
    current = atof(buffer);
    std::cout << "Power Supply: Current is " << current << "A" << std::endl;

ps_err_getCurrent:
    current = 0.0;
    return err;
}

PowerSupply::PsError PowerSupply::turnOn(void)
{
    PsError err = PsError::ERR_SUCCESS;

    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        goto err_turnOn;
    }

    /* Send turn on command */
    err = sendCommand(psCommands["turnOn"], "");
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to turn on power supply. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
    }
    else
    {
        std::cout << "Power Supply: Turned on" << std::endl;
    }

err_turnOn:
    return err;
}

PowerSupply::PsError PowerSupply::turnOff(void)
{
    PsError err = PsError::ERR_SUCCESS;

    /* Check if the instrument is open */
    if (this->isOpen() != PsError::ERR_SUCCESS) 
    {
        std::cout << "Power Supply: Device not connected" << std::endl;
        goto err_turnOff;
    }

    /* Send turn off command */
    err = sendCommand(psCommands["turnOff"], "");
    if (err != PsError::ERR_SUCCESS)
    {
        std::cout << "Failed to turn off power supply. Error: " << static_cast<int>(err) << std::endl;
        err = PsError::ERR_OPERATION_FAILED;
    }
    else
    {
        std::cout << "Power Supply: Turned off" << std::endl;
    }

err_turnOff:
    return err;
}

void PowerSupply::close(void)
{
    if (instrument != VI_NULL) 
    {
        viClose(instrument);
        instrument = VI_NULL;
    }
    if (defaultRM != VI_NULL)
    {
        viClose(defaultRM);
        defaultRM = VI_NULL;
    }
    port = "";
}
