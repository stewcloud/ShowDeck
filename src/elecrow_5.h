#pragma once

#ifdef SHOWDECK_ELECROW_5

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

class Elecrow5Display : public lgfx::LGFX_Device {
 public:
  Elecrow5Display();

 private:
  lgfx::Bus_RGB bus_;
  lgfx::Panel_RGB panel_;
};

void initElecrowTouch();
bool readElecrowTouch(int16_t &x, int16_t &y);

#endif
