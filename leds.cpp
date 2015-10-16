#include <Arduino.h>
#include <string.h>
#include "leds.h"

///////////////////////////////////////////////////////////////////////////
// LED
///////////////////////////////////////////////////////////////////////////

LED::LED() : _state(false) {}

boolean LED::getState() {
  return _state;
}

boolean LED::isRgb() {
  return false;
};

boolean LED::setColor(int red, int green, int blue) {
  return false;
};

///////////////////////////////////////////////////////////////////////////
// MonochromeLED
///////////////////////////////////////////////////////////////////////////

MonochromeLED::MonochromeLED(int gpioPin) :
  _pin(gpioPin) {
  
}

void MonochromeLED::setup() {
  pinMode(_pin, OUTPUT);
  setState(false);
}

void MonochromeLED::setState(boolean state) {
  _state = state;
  int value = state ? HIGH : LOW;
  digitalWrite(_pin, value);
}

String MonochromeLED::getLedInfo(int index) {
  String rv("{\"d\":");
  rv += String(index);
  rv += ",\"isRgb\":false,\"s\":\"";
  rv += (_state) ? "on" : "off";
  rv += "\"}";
  return rv;
}

///////////////////////////////////////////////////////////////////////////
// RgbLED
///////////////////////////////////////////////////////////////////////////

RgbLED::RgbLED(int gpioRPin, int gpioGPin, int gpioBPin) :
  _rPin(gpioRPin), _gPin(gpioGPin), _bPin(gpioBPin),
  _red(0xff), _green(0xff), _blue(0xff) {
    
}

void RgbLED::setup() {
  pinMode(_rPin, OUTPUT);
  pinMode(_gPin, OUTPUT);
  pinMode(_bPin, OUTPUT);
  setState(false);
}

void RgbLED::setState(boolean state) {
  int red = _red, green = _green, blue = _blue;
  _state = state;

  if (state) {
    if (_red == 0 && _green == 0 && _blue == 0) {
      // pick a random color
      red = random(0, 0xff);
      green = random(0, 0xff);
      blue = random(0, 0xff);
    }
  }
  else {
    red = green = blue = 0;
  }

  analogWrite(_rPin, red);
  analogWrite(_gPin, green);
  analogWrite(_bPin, blue);
}

boolean RgbLED::isRgb() { return true; }

boolean RgbLED::setColor(int red, int green, int blue) {
  if (red < 0 || red > 0xff ||
      green < 0 || green > 0xff ||
      blue < 0 || blue > 0xff) {
    return false;
  }

  _red = red;
  _green = green;
  _blue = blue;

  if (_state) {
    setState(_state);
  }
  
  return true;
}

String RgbLED::getLedInfo(int index) {
  String rv("{\"d\":");
  rv += String(index);
  rv += ",\"isRgb\":true,\"s\":\"";
  rv += (_state) ? "on" : "off";
  rv += "\",\"r\":";
  rv += String(_red);
  rv += ",\"g\":";
  rv += String(_green);
  rv += ",\"b\":";
  rv += String(_blue);
  rv += "}";
  return rv;    
}

///////////////////////////////////////////////////////////////////////////
// LEDGroup
///////////////////////////////////////////////////////////////////////////

LEDGroup::LEDGroup(LED** leds, int numLeds, Ticker::callback_t modeStepper) :
  _leds(leds), _numLeds(numLeds), _mode(NONE), _modeStepper(modeStepper) {
  
}

void LEDGroup::setup() {
  for (int i = 0; i < _numLeds; i++) {
    _leds[i]->setup();
  }
}

String LEDGroup::getGroupInfo() {
  String rv("{\"count\":");
  rv += String(size());
  rv += ",\"m\":\"";
  rv += getModeString(_mode);
  rv += "\"}";
  return rv;
}

String LEDGroup::getGroupEnumeration() {
  String rv("[");
  for (int i = 0; i < _numLeds; i++) {
    if (i != 0) {
      rv += ",";
    }
    rv += _leds[i]->getLedInfo(i);
  }
  rv += "]";
  return rv;
}

String LEDGroup::getLedInfo(int id) {
  return _leds[id]->getLedInfo(id);  
}

boolean LEDGroup::setMode(String mode) {
  modes newMode = getModeFromString(mode);
  if (newMode != NUM_MODES) {
    _mode = newMode;

    switch (_mode) {
    case NONE:
      // Stop the ticker and turn off all LEDs
      _modeTicker.detach();
      setAllState(false);
      break;
    case ALL:
      // Stop the ticker and turn on all LEDs
      _modeTicker.detach();
      setAllState(true);
    case BLINK:
      _modeFlashState = false;
      _modeTicker.attach_ms(500, _modeStepper);
      break;
    case CHASE:
      _modeCurrentState = 5; // LED 1 and 3
      _modeTicker.attach_ms(250, _modeStepper);
      break;
    case TWINKLE:
      _modeTwinkleCounter = 0;
      _modeCurrentState = 0;
      _modeNextState = random(0, _numLeds);
      _leds[_modeCurrentState]->setState(true);
      _modeTicker.attach_ms(100, _modeStepper);
      break;
    }
    return true;
  }
  return false;
}

void LEDGroup::modeStep() {
  switch (_mode) {
  case BLINK:
    _modeFlashState = !_modeFlashState;
    setAllState(_modeFlashState);
    break;
  case CHASE:
    _modeCurrentState = _modeCurrentState << 1;
    _modeCurrentState |= (_modeCurrentState & (1 << _numLeds)) >> _numLeds;
    
    for (int i = 0; i < _numLeds; i++) {
      _leds[i]->setState((_modeCurrentState & (1 << i)) != 0);
    }
    break;
  case TWINKLE:
    // |0 1 2|0 1 2|0 1 2|0 1 2|0
    // |X X X X     X X X X     X
    // |X     X X X X     X X X X
    _modeTwinkleCounter++;
    switch (_modeTwinkleCounter % 3) {
    case 0:
      while ((_modeNextState = random(0, _numLeds)) == _modeCurrentState);
      _leds[_modeNextState]->setState(true);
      break;
    case 1:
      _leds[_modeCurrentState]->setState(false);
      _modeCurrentState = _modeNextState;
      break;
    }
  }
}

void LEDGroup::setAllState(boolean state) {
  for (int i = 0; i < _numLeds; i++) {
    _leds[i]->setState(state);
  }    
}

int LEDGroup::size() {
  return _numLeds;
}

LED* LEDGroup::operator[](int index) {
  if (index >= 0 && index < _numLeds) {
    return _leds[index];
  }
  return NULL;
}

// must stay in sync with 'modes' enum
const char* LEDGroup::_modeStrings[5] = {
  "none",
  "blink",
  "chase",
  "twinkle",
  "all"
};

const char* LEDGroup::getModeString(modes m) {
  if (m < 0 || m >= NUM_MODES) {
    return "";
  }
  return _modeStrings[m];
}

LEDGroup::modes LEDGroup::getModeFromString(const String& modeStr) {
  for (int i = 0; i < NUM_MODES; i++) {
    if (modeStr == _modeStrings[i]) {
      return (modes)i;
    }
  }
  return NUM_MODES;
}

