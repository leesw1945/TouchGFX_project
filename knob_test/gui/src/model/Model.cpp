#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <touchgfx/Utils.hpp>
#include <gui/common/MenuItems.hpp>

#ifdef KNOB_INTERFACE
#include "knob_interface.hpp"
#endif

Model::Model() :
    modelListener(0)
{
}

void Model::tick()
{
    tickCount++;

    // auto-increment the battery level (only for demo animation purpose)
    if (batteryLevel < 1000)
    {
        batteryLevel += 2;
    }
}

void Model::setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    touchgfx_printf("Ambient light set to: Red = %i, Green = %i, Blue = %i\r\n", red, green, blue); // Print to TouchGFX simulator debug log

#ifdef KNOB_INTERFACE
    knobSetAmbientLightRGB(red, green, blue);
#endif
}

void Model::sendCommand(char type, char value)
{
    uint8_t command[] = "X:000\r\n";
    command[0] = type;
    command[2] = '0' + value / 100;
    command[3] = '0' + (value % 100) / 10;
    command[4] = '0' + value % 10;

    // Print to TouchGFX simulator debug log
    touchgfx_printf("Transmitted on UART: ");
    touchgfx_printf((char*)command);

#ifdef KNOB_INTERFACE
    knobUartSendString(command);
#endif
}

void Model::setSelectedDemoNumber(int value)
{
    selectedDemoNumber = value;

    // reset battery level when battery screen is entered (only for demo animation purpose)
    if (menuItems[value].titleId == T_MENU_BATTERY)
    {
        batteryLevel = 0;
    }
}

int Model::getSelectedDemoNumber()
{
    return selectedDemoNumber;
}

void Model::setVolume(int value)
{
    volume = value;
    sendCommand('V', volume);
}

int Model::getVolume()
{
    return volume;
}

void Model::setPressure(int value)
{
    pressure = value;
    sendCommand('P', pressure);
}

int Model::getPressure()
{
    return pressure;
}

void Model::setBrightness(int value)
{
    brightness = value;
    sendCommand('B', brightness);
}

int Model::getBrightness()
{
    return brightness;
}

void Model::setTemperature(int value)
{
    temperature = value;
    sendCommand('T', temperature);
}

int Model::getTemperature()
{
    return temperature;
}

int Model::getBatteryLevel()
{
    return batteryLevel;
}
