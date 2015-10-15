#ifndef __LEDS_H
#define __LEDS_H

#include <stdint.h>
#include <Ticker.h>

// Color constants in the form needed to fill in parameters to setColor() method
#define COLOR_RANDOM  0x00, 0x00, 0x00
#define COLOR_RED     0xff, 0x00, 0x00
#define COLOR_ORANGE  0xff, 0x8c, 0x00
#define COLOR_YELLOW  0xff, 0xff, 0x00
#define COLOR_GREEN   0x00, 0xff, 0x00
#define COLOR_BLUE    0x00, 0x00, 0xff
#define COLOR_PURPLE  0x80, 0x00, 0x80
#define COLOR_WHITE   0xff, 0xff, 0xff

class LED {
public:
  LED();
  
  virtual void setup() = 0;
  virtual void setState(boolean state) = 0;
  virtual boolean getState();
  virtual boolean isRgb();
  virtual boolean setColor(int red, int green, int blue);
  virtual String getLedInfo(int index) = 0;
  
protected:
  boolean _state;
};

class MonochromeLED : public LED {
public:
  MonochromeLED(int gpioPin);

  void setup();
  void setState(boolean state);
  String getLedInfo(int index);
  
protected:
  int _pin;
};

class RgbLED : public LED {
public:
  RgbLED(int gpioRPin, int gpioGPin, int gpioBPin);

  void setup();
  void setState(boolean state);
  boolean isRgb();
  boolean setColor(int red, int green, int blue);
  String getLedInfo(int index);
  
protected:
  int _rPin;
  int _gPin;
  int _bPin;
  int _red;
  int _green;
  int _blue;
};

class LEDGroup {
public:
  LEDGroup(LED** leds, int numLeds, Ticker::callback_t modeStepper);

  void setup();
  String getGroupInfo();
  String getGroupEnumeration();
  String getLedInfo(int id);
  
  enum modes {
    NONE = 0,
    BLINK = 1,
    CHASE = 2,
    TWINKLE = 3,
    ALL = 4,
    NUM_MODES
  };

  boolean setMode(String mode);
  void modeStep();
  void setAllState(boolean state);
  int size();
  LED* operator[](int index);
  
private:
  // must stay in sync with 'modes' enum
  static const char* _modeStrings[5];

  const char* getModeString(modes m);

  modes getModeFromString(const String& modeStr);
  
  LED** _leds;
  int _numLeds;
  modes _mode;
  Ticker::callback_t _modeStepper;
  Ticker _modeTicker;
  boolean _modeFlashState;
  int _modeCurrentState;
  int _modeNextState;
  int _modeTwinkleCounter;
};

#endif // __LEDS_H
