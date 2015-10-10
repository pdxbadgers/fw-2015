#ifndef __CONFIG_SPACE_H
#define __CONFIG_SPACE_H

#include <WString.h>

class ConfigSpace
{
public:
  ConfigSpace();

  void begin();
  bool speaker();
  void setSpeaker(bool spkr);
  size_t nameLen();
  String name();
  void setName(const String &buf);

private:
  bool initialized;
  bool untouched();
  void touch();
};

extern ConfigSpace configSpace;

#endif

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
