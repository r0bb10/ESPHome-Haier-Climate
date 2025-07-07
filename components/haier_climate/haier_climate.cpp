#include "haier_climate.h"

namespace esphome {
namespace haier_climate {

HaierClimate::HaierClimate(uart::UARTComponent *parent)
    : uart::UARTDevice(parent) {}

void HaierClimate::setup() {
  // Nothing needed here for now.
}

void HaierClimate::loop() {
  // Read serial data from AC
  while (available() >= LEN_B) {
    uint8_t rx[LEN_B];
    read_array(rx, LEN_B);
    if (rx[LEN_B-1] != inCheck_) {
      inCheck_ = rx[LEN_B-1];
      memcpy(data_, rx, LEN_B);
      parse_data(data_);
    }
    // flush remaining bytes if any
    while (available()) read();
  }

  // Poll AC every 5 seconds
  unsigned long now = millis();
  if (now - last_poll_ > 5000) {
    last_poll_ = now;
    send_data(qstn_, sizeof(qstn_));
  }
}

climate::ClimateTraits HaierClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({
    climate::CLIMATE_MODE_OFF,
    climate::CLIMATE_MODE_COOL,
    climate::CLIMATE_MODE_HEAT,
    climate::CLIMATE_MODE_DRY,
    climate::CLIMATE_MODE_FAN_ONLY,
    climate::CLIMATE_MODE_AUTO
  });
  traits.set_supported_fan_modes({
    climate::CLIMATE_FAN_AUTO,
    climate::CLIMATE_FAN_LOW,
    climate::CLIMATE_FAN_MEDIUM,
    climate::CLIMATE_FAN_HIGH
  });
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(16);
  traits.set_visual_max_temperature(30);
  traits.set_visual_temperature_step(1);
  return traits;
}

void HaierClimate::parse_data(const uint8_t *data) {
  // Set current temperature
  this->current_temperature = data[B_CUR_TMP];
  // Set target temperature (offset by 16 as in original)
  this->target_temperature = data[B_SET_TMP] + 16;

  // Mode
  climate::ClimateMode mode = climate::CLIMATE_MODE_OFF;
  if (data[B_POWER] == 0x01 || data[B_POWER] == 0x11) {
    switch (data[B_MODE]) {
      case 0x00: mode = climate::CLIMATE_MODE_AUTO; break;
      case 0x01: mode = climate::CLIMATE_MODE_COOL; break;
      case 0x02: mode = climate::CLIMATE_MODE_HEAT; break;
      case 0x03: mode = climate::CLIMATE_MODE_FAN_ONLY; break;
      case 0x04: mode = climate::CLIMATE_MODE_DRY; break;
      default: break;
    }
  }
  this->mode = mode;

  // Fan speed
  switch (data[B_FAN_SPD]) {
    case 0x00: this->fan_mode = climate::CLIMATE_FAN_HIGH; break;
    case 0x01: this->fan_mode = climate::CLIMATE_FAN_MEDIUM; break;
    case 0x02: this->fan_mode = climate::CLIMATE_FAN_LOW; break;
    case 0x03: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
    default: this->fan_mode = climate::CLIMATE_FAN_AUTO; break;
  }

  this->publish_state();
}

void HaierClimate::send_data(const uint8_t *req, size_t size) {
  write_array(req, size - 1);
  write_byte(get_crc(req, size - 1));
}

uint8_t HaierClimate::get_crc(const uint8_t *req, size_t size) {
  uint8_t crc = 0;
  for (size_t i = 2; i < size; i++) {
    crc += req[i];
  }
  return crc;
}

void HaierClimate::control(const climate::ClimateCall &call) {
  // Setpoint
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    int t = roundf(temp);
    if (t >= 16 && t <= 30) {
      data_[B_SET_TMP] = t - 16;
    }
  }

  // Mode
  if (call.get_mode().has_value()) {
    switch (*call.get_mode()) {
      case climate::CLIMATE_MODE_OFF:
        send_data(off_, sizeof(off_));
        return;
      case climate::CLIMATE_MODE_COOL: data_[B_MODE] = 1; break;
      case climate::CLIMATE_MODE_HEAT: data_[B_MODE] = 2; break;
      case climate::CLIMATE_MODE_DRY: data_[B_MODE] = 4; break;
      case climate::CLIMATE_MODE_FAN_ONLY: data_[B_MODE] = 3; break;
      case climate::CLIMATE_MODE_AUTO: data_[B_MODE] = 0; break;
      default: break;
    }
  }

  // Fan mode
  if (call.get_fan_mode().has_value()) {
    switch (*call.get_fan_mode()) {
      case climate::CLIMATE_FAN_AUTO:   data_[B_FAN_SPD] = 3; break;
      case climate::CLIMATE_FAN_LOW:    data_[B_FAN_SPD] = 2; break;
      case climate::CLIMATE_FAN_MEDIUM: data_[B_FAN_SPD] = 1; break;
      case climate::CLIMATE_FAN_HIGH:   data_[B_FAN_SPD] = 0; break;
      default: break;
    }
  }

  // Power (if mode or setpoint changed, send ON)
  send_data(on_, sizeof(on_));
  // Then send the data frame with updated values
  data_[B_CMD] = 0;
  data_[9] = 1;
  data_[10] = 77;
  data_[11] = 95;
  send_data(data_, LEN_B);
}

}  // namespace haier_climate
}  // namespace esphome