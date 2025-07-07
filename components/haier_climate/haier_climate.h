#pragma once
#include "esphome.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace haier_climate {

class HaierClimate : public climate::Climate, public Component, public uart::UARTDevice {
 public:
  HaierClimate(uart::UARTComponent *parent);

  void setup() override;
  void loop() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  // Protocol indices (matching original INO defines)
  static constexpr size_t LEN_B   = 37;
  static constexpr size_t B_CUR_TMP   = 13;
  static constexpr size_t B_CMD       = 17;
  static constexpr size_t B_MODE      = 23;
  static constexpr size_t B_FAN_SPD   = 25;
  static constexpr size_t B_SWING     = 27;
  static constexpr size_t B_LOCK_REM  = 28;
  static constexpr size_t B_POWER     = 29;
  static constexpr size_t B_FRESH     = 31;
  static constexpr size_t B_SET_TMP   = 35;

  uint8_t data_[LEN_B]{};
  uint8_t inCheck_{0};
  unsigned long last_poll_{0};

  void parse_data(const uint8_t *data);
  void send_data(const uint8_t *req, size_t size);
  uint8_t get_crc(const uint8_t *req, size_t size);

  // Command templates
  static constexpr uint8_t qstn_[13] = {255,255,10,0,0,0,0,0,1,1,77,1,90};
  static constexpr uint8_t on_[13]   = {255,255,10,0,0,0,0,0,1,1,77,2,91};
  static constexpr uint8_t off_[13]  = {255,255,10,0,0,0,0,0,1,1,77,3,92};
};

}  // namespace haier_climate
}  // namespace esphome