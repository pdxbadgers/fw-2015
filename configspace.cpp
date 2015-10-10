#include "configspace.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#define CONFIG_SIZE  512
#define MAGIC_SIZE   4
#define SPKR_ADDR    4
#define NAME_ADDR    5
#define MAX_NAME_LEN (CONFIG_SIZE - MAGIC_SIZE - 1 - 1 - 1)

ConfigSpace configSpace;

ConfigSpace::ConfigSpace() : initialized(false)
{
}

void ConfigSpace::begin()
{
  EEPROM.begin(CONFIG_SIZE);
  initialized = true;
  if (untouched()) {
    touch();
  };
}

bool ConfigSpace::speaker()
{
  if (!initialized) {
    return false;
  }
  return EEPROM.read(SPKR_ADDR);
}

void ConfigSpace::setSpeaker(bool spkr)
{
  if (!initialized) {
    return;
  }
  EEPROM.write(SPKR_ADDR, spkr);
  EEPROM.commit();
}

size_t ConfigSpace::nameLen()
{
  if (!initialized) {
    return 0;
  }
  for (size_t i = 0; i < MAX_NAME_LEN; i++) {
    if (EEPROM.read(NAME_ADDR + i) == 0) {
      return i;
    }
  }
  return 0; //should never happen
}

String ConfigSpace::name()
{
  String ret;
  if (!initialized) {
    return ret;
  }
  ret.reserve(nameLen() + 1);
  for (int i = NAME_ADDR; i < CONFIG_SIZE; i++) {
    const char c = EEPROM.read(i);
    ret += c;
    if (c == 0) {
      break;
    }
  }
  return ret;
}

void ConfigSpace::setName(const String &buf)
{
  if (!initialized) {
    return;
  }
  const int stor_len = min(MAX_NAME_LEN - 1, buf.length());
  for (int i = 0; i < stor_len; i++) {
    EEPROM.write(NAME_ADDR + i, buf[i]);
  }
  EEPROM.write(NAME_ADDR + stor_len, 0);
  EEPROM.commit();
}

bool ConfigSpace::untouched()
{
  if (EEPROM.read(0) != 0xFE) return false;
  if (EEPROM.read(1) != 0xED) return false;
  if (EEPROM.read(2) != 0xFA) return false;
  if (EEPROM.read(3) != 0xCE) return false;
  return true;
}

void ConfigSpace::touch()
{
  EEPROM.write(0, 0xFE);
  EEPROM.write(1, 0xED);
  EEPROM.write(2, 0xFA);
  EEPROM.write(3, 0xCE);
  for (int i = 4; i < CONFIG_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}


/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * mode: c++
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
