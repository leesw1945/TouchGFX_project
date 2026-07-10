#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    void setAmbientLightRGB(uint8_t red, uint8_t green, uint8_t blue);
    void sendCommand(char type, char value);
    void setSelectedDemoNumber(int value);
    int getSelectedDemoNumber();
    void setVolume(int value);
    int getVolume();
    void setPressure(int value);
    int getPressure();
    void setBrightness(int value);
    int getBrightness();
    void setTemperature(int value);
    int getTemperature();
    int getBatteryLevel();

protected:
    ModelListener* modelListener;
    uint32_t tickCount = 0;

    int selectedDemoNumber = 1;
    int pressure = 8;
    int volume = 12;
    int brightness = 20;
    int temperature = 20;
    int batteryLevel = 0;
};

#endif // MODEL_HPP
