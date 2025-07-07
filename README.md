# ESPHome Haier Climate (alpha stage)

This ESPHome external component provides native climate (thermostat) integration for Haier (and similar) air conditioners that use the UART protocol described in the original Arduino/ESP8266 project.

## Features

- Exposes your AC as a Home Assistant `climate` entity
- Control power, mode, setpoint, fan speed
- Reports current and target temperature
- Fully local, no cloud required

## Installation

Add to your `esphome` device YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/r0bb10/esphome-haier-climate.git
      ref: main
    components: [haier_climate]

uart:
  id: ac_uart
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 9600

climate:
  - platform: haier_climate
    id: haier_ac
    uart_id: ac_uart
    name: "Haier AC"
```

## Wiring

Connect your ESP device's TX/RX pins to the AC UART interface as in the original hardware setup.

## License

See [LICENSE](LICENSE).