#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#define TAG "JK-BMS"

#include "jk_bms.h"

namespace esphome {
namespace jk_bms {

//static const char *const TAG = "jk_bms";

static const uint8_t MAX_NO_RESPONSE_COUNT = 5;

static const uint8_t FUNCTION_READ_ALL = 0x06;
static const uint8_t ADDRESS_READ_ALL = 0x00;
static const uint8_t WRITE_REGISTER = 0x02;

static const uint8_t ERRORS_SIZE = 14;
static const char *const ERRORS[ERRORS_SIZE] = {
    "Low capacity",                              // Byte 0.0, warning
    "Power tube overtemperature",                // Byte 0.1, alarm
    "Charging overvoltage",                      // Byte 0.2, alarm
    "Discharging undervoltage",                  // Byte 0.3, alarm
    "Battery over temperature",                  // Byte 0.4, alarm
    "Charging overcurrent",                      // Byte 0.5, alarm
    "Discharging overcurrent",                   // Byte 0.6, alarm
    "Cell pressure difference",                  // Byte 0.7, alarm
    "Overtemperature alarm in the battery box",  // Byte 1.0, alarm
    "Battery low temperature",                   // Byte 1.1, alarm
    "Cell overvoltage",                          // Byte 1.2, alarm
    "Cell undervoltage",                         // Byte 1.3, alarm
    "309_A protection",                          // Byte 1.4, alarm
    "309_A protection",                          // Byte 1.5, alarm
};

static const uint8_t OPERATION_MODES_SIZE = 4;
static const char *const OPERATION_MODES[OPERATION_MODES_SIZE] = {
    "Charging enabled",     // 0x00
    "Discharging enabled",  // 0x01
    "Balancer enabled",     // 0x02
    "Battery dropped",      // 0x03
};

static const uint8_t BATTERY_TYPES_SIZE = 3;
static const char *const BATTERY_TYPES[BATTERY_TYPES_SIZE] = {
    "Lithium Iron Phosphate",  // 0x00
    "Ternary Lithium",         // 0x01
    "Lithium Titanate",        // 0x02
};

void JkBms::on_jk_modbus_data(const uint8_t &function, const std::vector<uint8_t> &data) {
  this->reset_online_status_tracker_();

  if (function == FUNCTION_READ_ALL) {
    this->on_status_data_(data);
    return;
  }

  ESP_LOGW(TAG, "Invalid size (%zu) for JK BMS frame!", data.size());
}

void JkBms::on_status_data_(const std::vector<uint8_t> &data) {
  auto jk_get_16bit = [&](size_t i) -> uint16_t { return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0); };
  auto jk_get_32bit = [&](size_t i) -> uint32_t {
    return (uint32_t(jk_get_16bit(i + 0)) << 16) | (uint32_t(jk_get_16bit(i + 2)) << 0);
  };

  ESP_LOGI(TAG, "Status frame received.");

  // Status request
  // -> 0x4E 0x57 0x00 0x13 0x00 0x00 0x00 0x00 0x06 0x03 0x00 0x00 0x00 0x00 0x00 0x00 0x68 0x00 0x00 0x01 0x29
  //
  // Status response
  // <- 0x4E 0x57 0x01 0x1B 0x00 0x00 0x00 0x00 0x06 0x00 0x01: Header
  //
  // *Data*
  //
  // Address Content: Description      Decoded content                         Coeff./Unit
  // 0x79: Individual Cell voltage
  // 0x2A: Cell count               42 / 3 bytes = 14 cells
  // 0x01 0x0E 0xED: Cell 1         3821 * 0.001 = 3.821V                        0.001 V
  // 0x02 0x0E 0xFA: Cell 2         3834 * 0.001 = 3.834V                        0.001 V
  // 0x03 0x0E 0xF7: Cell 3         3831 * 0.001 = 3.831V                        0.001 V
  // 0x04 0x0E 0xEC: Cell 4         ...                                          0.001 V
  // 0x05 0x0E 0xF8: Cell 5         ...                                          0.001 V
  // 0x06 0x0E 0xFA: Cell 6         ...                                          0.001 V
  // 0x07 0x0E 0xF1: Cell 7         ...                                          0.001 V
  // 0x08 0x0E 0xF8: Cell 8         ...                                          0.001 V
  // 0x09 0x0E 0xE3: Cell 9         ...                                          0.001 V
  // 0x0A 0x0E 0xFA: Cell 10        ...                                          0.001 V
  // 0x0B 0x0E 0xF1: Cell 11        ...                                          0.001 V
  // 0x0C 0x0E 0xFB: Cell 12        ...                                          0.001 V
  // 0x0D 0x0E 0xFB: Cell 13        ...                                          0.001 V
  // 0x0E 0x0E 0xF2: Cell 14        3826 * 0.001 = 3.826V                        0.001 V
  uint8_t cells = data[1] / 3;
  cell_count_ = cells;

  float min_cell_voltage = 100.0f;
  float max_cell_voltage = -100.0f;
  float average_cell_voltage = 0.0f;
  uint8_t min_voltage_cell = 0;
  uint8_t max_voltage_cell = 0;
  for (uint8_t i = 0; i < cells; i++) {
    float cell_voltage = (float) jk_get_16bit(i * 3 + 3) * 0.001f;
    average_cell_voltage = average_cell_voltage + cell_voltage;
    if (cell_voltage < min_cell_voltage) {
      min_cell_voltage = cell_voltage;
      min_voltage_cell = i + 1;
    }
    if (cell_voltage > max_cell_voltage) {
      max_cell_voltage = cell_voltage;
      max_voltage_cell = i + 1;
    }
    this->cells_[i].cell_voltage_sensor_ = cell_voltage;
  }
  average_cell_voltage = average_cell_voltage / cells;

  this->min_cell_voltage_sensor_ = min_cell_voltage;
  this->max_cell_voltage_sensor_ = max_cell_voltage;
  this->max_voltage_cell_sensor_ = (float) max_voltage_cell;
  this->min_voltage_cell_sensor_ = (float) min_voltage_cell;
  this->delta_cell_voltage_sensor_ = max_cell_voltage - min_cell_voltage;
  this->average_cell_voltage_sensor_ = average_cell_voltage;

  uint16_t offset = data[1] + 3;

  // 0x80 0x00 0x1D: Read power tube temperature                 29°C                      1.0 °C
  // --->  99 = 99°C, 100 = 100°C, 101 = -1°C, 140 = -40°C
  this->power_tube_temperature_sensor_ = get_temperature_(jk_get_16bit(offset + 3 * 0));

  // 0x81 0x00 0x1E: Read the temperature in the battery box     30°C                      1.0 °C
  this->temperature_sensor_1_sensor_ = get_temperature_(jk_get_16bit(offset + 3 * 1));
  this->temperature_sensors_[0].temperature_sensor_ = this->temperature_sensor_1_sensor_;

  // 0x82 0x00 0x1C: Read battery temperature                    28°C                      1.0 °C
  this->temperature_sensor_2_sensor_ = get_temperature_(jk_get_16bit(offset + 3 * 2));
  this->temperature_sensors_[1].temperature_sensor_ = this->temperature_sensor_2_sensor_;

  // 0x83 0x14 0xEF: Total battery voltage                       5359 * 0.01 = 53.59V      0.01 V
  float total_voltage = (float) jk_get_16bit(offset + 3 * 3) * 0.01f;
  this->total_voltage_sensor_ = total_voltage;

  // 0x84 0x80 0xD0: Current data                                32976                     0.01 A
  // this->publish_state_(this->current_sensor_, get_current_(jk_get_16bit(offset + 3 * 4), 0x01) * 0.01f);
  float current = get_current_(jk_get_16bit(offset + 3 * 4), data[offset + 84 + 3 * 45]) * 0.01f;
  this->current_sensor_ = current;
  this->charging_current_sensor_ = std::max(0.0f, current);
  this->discharging_current_sensor_ = std::abs(std::min(0.0f, current));

  float power = total_voltage * current;
  this->power_sensor_ = power;
  this->charging_power_sensor_ = std::max(0.0f, power);               // 500W vs 0W -> 500W
  this->discharging_power_sensor_ = std::abs(std::min(0.0f, power));  // -500W vs 0W -> 500W

  // 0x85 0x0F: Battery remaining capacity                       15 %
  uint8_t raw_battery_remaining_capacity = data[offset + 3 * 5];
  this->capacity_remaining_sensor_ = (float) raw_battery_remaining_capacity;

  // 0x86 0x02: Number of battery temperature sensors             2                        1.0  count
  this->temperature_sensors_sensor_ = data[offset + 2 + 3 * 5];

  // 0x87 0x00 0x04: Number of battery cycles                     4                        1.0  count
  this->charging_cycles_sensor_ = (float) jk_get_16bit(offset + 4 + 3 * 5);

  // 0x89 0x00 0x00 0x00 0x00: Total battery cycle capacity
  this->total_charging_cycle_capacity_sensor_ = (float) jk_get_32bit(offset + 4 + 3 * 6);

  // 0x8A 0x00 0x0E: Total number of battery strings             14                        1.0  count
  this->battery_strings_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 7);

  // 0x8B 0x00 0x00: Battery warning message                     0000 0000 0000 0000
  //
  // Bit 0    Low capacity                                1 (alarm), 0 (normal)    warning
  // Bit 1    Power tube overtemperature                  1 (alarm), 0 (normal)    alarm
  // Bit 2    Charging overvoltage                        1 (alarm), 0 (normal)    alarm
  // Bit 3    Discharging undervoltage                    1 (alarm), 0 (normal)    alarm
  // Bit 4    Battery over temperature                    1 (alarm), 0 (normal)    alarm
  // Bit 5    Charging overcurrent                        1 (alarm), 0 (normal)    alarm
  // Bit 6    Discharging overcurrent                     1 (alarm), 0 (normal)    alarm
  // Bit 7    Cell pressure difference                    1 (alarm), 0 (normal)    alarm
  // Bit 8    Overtemperature alarm in the battery box    1 (alarm), 0 (normal)    alarm
  // Bit 9    Battery low temperature                     1 (alarm), 0 (normal)    alarm
  // Bit 10   Cell overvoltage                            1 (alarm), 0 (normal)    alarm
  // Bit 11   Cell undervoltage                           1 (alarm), 0 (normal)    alarm
  // Bit 12   309_A protection                            1 (alarm), 0 (normal)    alarm
  // Bit 13   309_A protection                            1 (alarm), 0 (normal)    alarm
  // Bit 14   Reserved
  // Bit 15   Reserved
  //
  // Examples:
  // 0x0001 = 00000000 00000001: Low capacity alarm
  // 0x0002 = 00000000 00000010: MOS tube over-temperature alarm
  // 0x0003 = 00000000 00000011: Low capacity alarm AND power tube over-temperature alarm
  uint16_t raw_errors_bitmask = jk_get_16bit(offset + 6 + 3 * 8);
  errors_bitmask_sensor_ = raw_errors_bitmask;
  this->errors_text_sensor_ = this->error_bits_to_string_(raw_errors_bitmask);

  // 0x8C 0x00 0x07: Battery status information                  0000 0000 0000 0111
  // Bit 0: Charging enabled        1 (on), 0 (off)
  // Bit 1: Discharging enabled     1 (on), 0 (off)
  // Bit 2: Balancer enabled        1 (on), 0 (off)
  // Bit 3: Battery dropped(?)      1 (normal), 0 (offline)
  // Bit 4...15: Reserved

  // Example: 0000 0000 0000 0111 -> Charging + Discharging + Balancer enabled
  uint16_t raw_modes_bitmask = jk_get_16bit(offset + 6 + 3 * 9);
  this->operation_mode_bitmask_sensor_ = raw_modes_bitmask;

  this->operation_mode_text_sensor_ = this->mode_bits_to_string_(raw_modes_bitmask);
  this->charging_binary_sensor_ = check_bit_(raw_modes_bitmask, 1);
  this->discharging_binary_sensor_ = check_bit_(raw_modes_bitmask, 2);
  this->balancing_binary_sensor_ = check_bit_(raw_modes_bitmask, 4);

  // 0x8E 0x16 0x26: Total voltage overvoltage protection        5670 * 0.01 = 56.70V     0.01 V
  this->total_voltage_overvoltage_protection_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 10) * 0.01f;

  // 0x8F 0x10 0xAE: Total voltage undervoltage protection       4270 * 0.01 = 42.70V     0.01 V
  this->total_voltage_undervoltage_protection_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 11) * 0.01f;

  // 0x90 0x0F 0xD2: Cell overvoltage protection voltage         4050 * 0.001 = 4.050V     0.001 V
  this->cell_voltage_overvoltage_protection_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 12) * 0.001f;

  // 0x91 0x0F 0xA0: Cell overvoltage recovery voltage           4000 * 0.001 = 4.000V     0.001 V
  this->cell_voltage_overvoltage_recovery_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 13) * 0.001f;

  // 0x92 0x00 0x05: Cell overvoltage protection delay            5s                         1.0 s
  this->cell_voltage_overvoltage_delay_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 14);

  // 0x93 0x0B 0xEA: Cell undervoltage protection voltage        3050 * 0.001 = 3.050V     0.001 V
  this->cell_voltage_undervoltage_protection_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 15) * 0.001f;

  // 0x94 0x0C 0x1C: Cell undervoltage recovery voltage          3100 * 0.001 = 3.100V     0.001 V
  this->cell_voltage_undervoltage_recovery_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 16) * 0.001f;

  // 0x95 0x00 0x05: Cell undervoltage protection delay           5s                         1.0 s
  this->cell_voltage_undervoltage_delay_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 17);

  // 0x96 0x01 0x2C: Cell pressure difference protection value    300 * 0.001 = 0.300V     0.001 V     0.000-1.000V
  this->cell_pressure_difference_protection_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 18) * 0.001f;

  // 0x97 0x00 0x07: Discharge overcurrent protection value       7A                         1.0 A
  this->discharging_overcurrent_protection_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 19);

  // 0x98 0x00 0x03: Discharge overcurrent delay                  3s                         1.0 s
  this->discharging_overcurrent_delay_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 20);

  // 0x99 0x00 0x05: Charging overcurrent protection value        5A                         1.0 A
  this->charging_overcurrent_protection_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 21);

  // 0x9A 0x00 0x05: Charge overcurrent delay                     5s                         1.0 s
  this->charging_overcurrent_delay_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 22);

  // 0x9B 0x0C 0xE4: Balanced starting voltage                   3300 * 0.001 = 3.300V     0.001 V
  this->balance_starting_voltage_sensor_ = (float) jk_get_16bit(offset + 6 + 3 * 23) * 0.001f;

  // 0x9C 0x00 0x08: Balanced opening pressure difference           8 * 0.001 = 0.008V     0.001 V     0.01-1V
  this->balance_opening_pressure_difference_sensor_ =
                       (float) jk_get_16bit(offset + 6 + 3 * 24) * 0.001f;

  // 0x9D 0x01: Active balance switch                              1 (on)                     Bool     0 (off), 1 (on)
  this->balancing_switch_binary_sensor_ = (bool) data[offset + 6 + 3 * 25];

  // 0x9E 0x00 0x5A: Power tube temperature protection value                90°C            1.0 °C     0-100°C
  this->power_tube_temperature_protection_sensor_ = (float) jk_get_16bit(offset + 8 + 3 * 25);

  // 0x9F 0x00 0x46: Power tube temperature recovery value                  70°C            1.0 °C     0-100°C
  this->power_tube_temperature_recovery_sensor_ = (float) jk_get_16bit(offset + 8 + 3 * 26);

  // 0xA0 0x00 0x64: Temperature protection value in the battery box       100°C            1.0 °C     40-100°C
  this->temperature_sensor_temperature_protection_sensor_ =
                       (float) jk_get_16bit(offset + 8 + 3 * 27);

  // 0xA1 0x00 0x64: Temperature recovery value in the battery box         100°C            1.0 °C     40-100°C
  this->temperature_sensor_temperature_recovery_sensor_ =
                       (float) jk_get_16bit(offset + 8 + 3 * 28);

  // 0xA2 0x00 0x14: Battery temperature difference protection value        20°C            1.0 °C     5-10°C
  this->temperature_sensor_temperature_difference_protection_sensor_ =
                       (float) jk_get_16bit(offset + 8 + 3 * 29);

  // 0xA3 0x00 0x46: Battery charging high temperature protection value     70°C            1.0 °C     0-100°C
  this->charging_high_temperature_protection_sensor_ = (float) jk_get_16bit(offset + 8 + 3 * 30);

  // 0xA4 0x00 0x46: Battery discharge high temperature protection value    70°C            1.0 °C     0-100°C
  this->discharging_high_temperature_protection_sensor_ =
                       (float) jk_get_16bit(offset + 8 + 3 * 31);

  // 0xA5 0xFF 0xEC: Charging low temperature protection value             -20°C            1.0 °C     -45...25°C
  this->charging_low_temperature_protection_sensor_ =
                       (float) (int16_t) jk_get_16bit(offset + 8 + 3 * 32);

  // 0xA6 0xFF 0xF6: Charging low temperature protection recovery value    -10°C            1.0 °C     -45...25°C
  this->charging_low_temperature_recovery_sensor_ =
                       (float) (int16_t) jk_get_16bit(offset + 8 + 3 * 33);

  // 0xA7 0xFF 0xEC: Discharge low temperature protection value            -20°C            1.0 °C     -45...25°C
  this->discharging_low_temperature_protection_sensor_ =
                       (float) (int16_t) jk_get_16bit(offset + 8 + 3 * 34);

  // 0xA8 0xFF 0xF6: Discharge low temperature protection recovery value   -10°C            1.0 °C     -45...25°C
  this->discharging_low_temperature_recovery_sensor_ =
                       (float) (int16_t) jk_get_16bit(offset + 8 + 3 * 35);

  // 0xA9 0x0E: Battery string setting                                      14              1.0 count
  // this->publish_state_(this->battery_string_setting_sensor_, (float) data[offset + 8 + 3 * 36]);
  // 0xAA 0x00 0x00 0x02 0x30: Total battery capacity setting              560 Ah           1.0 Ah
  uint32_t raw_total_battery_capacity_setting = jk_get_32bit(offset + 10 + 3 * 36);
  this->total_battery_capacity_setting_sensor_ = (float) raw_total_battery_capacity_setting;
  this->capacity_remaining_derived_sensor_ =
                       (float) (raw_total_battery_capacity_setting * (raw_battery_remaining_capacity * 0.01f));

  // 0xAB 0x01: Charging MOS tube switch                                     1 (on)         Bool       0 (off), 1 (on)
  this->charging_switch_binary_sensor_ = (bool) data[offset + 15 + 3 * 36];

  // 0xAC 0x01: Discharge MOS tube switch                                    1 (on)         Bool       0 (off), 1 (on)
  this->discharging_switch_binary_sensor_ = (bool) data[offset + 17 + 3 * 36];

  // 0xAD 0x04 0x11: Current calibration                       1041mA * 0.001 = 1.041A     0.001 A     0.1-2.0A
  this->current_calibration_sensor_ = (float) jk_get_16bit(offset + 19 + 3 * 36) * 0.001f;

  // 0xAE 0x01: Protection board address                                     1              1.0
  this->device_address_sensor_ = (float) data[offset + 19 + 3 * 37];

  // 0xAF 0x01: Battery Type                                                 1              1.0
  // ---> 0 (lithium iron phosphate), 1 (ternary), 2 (lithium titanate)
  uint8_t raw_battery_type = data[offset + 21 + 3 * 37];
  if (raw_battery_type < BATTERY_TYPES_SIZE) {
    this->battery_type_text_sensor_ = BATTERY_TYPES[raw_battery_type];
  } else {
    this->battery_type_text_sensor_ = "Unknown";
  }

  // 0xB0 0x00 0x0A: Sleep waiting time                                      10s            1.0 s
  this->sleep_wait_time_sensor_ = (float) jk_get_16bit(offset + 23 + 3 * 37);

  // 0xB1 0x14: Low volume alarm                                             20%            1.0 %      0-80%
  this->alarm_low_volume_sensor_ = (float) data[offset + 23 + 3 * 38];

  // 0xB2 0x31 0x32 0x33 0x34 0x35 0x36 0x00 0x00 0x00 0x00: Modify parameter password
  this->password_text_sensor_ =
                       std::string(data.begin() + offset + 25 + 3 * 38, data.begin() + offset + 35 + 3 * 38);

  // 0xB3 0x00: Dedicated charger switch                                     1 (on)         Bool       0 (off), 1 (on)
  this->dedicated_charger_switch_binary_sensor_ = (bool) data[offset + 36 + 3 * 38];

  // 0xB4 0x49 0x6E 0x70 0x75 0x74 0x20 0x55 0x73: Device ID code
  this->device_type_text_sensor_ =
                       std::string(data.begin() + offset + 38 + 3 * 38, data.begin() + offset + 46 + 3 * 38);

  // 0xB5 0x32 0x31 0x30 0x31: Date of manufacture
  // 0xB6 0x00 0x00 0xE2 0x00: System working hours
  this->total_runtime_sensor_ = (float) jk_get_32bit(offset + 46 + 3 * 40) * 0.0166666666667;
  this->total_runtime_formatted_text_sensor_ =
                       format_total_runtime_(jk_get_32bit(offset + 46 + 3 * 40) * 60);

  // 0xB7 0x48 0x36 0x2E 0x58 0x5F 0x5F 0x53
  //      0x36 0x2E 0x31 0x2E 0x33 0x53 0x5F 0x5F: Software version number
  this->software_version_text_sensor_ =
                       std::string(data.begin() + offset + 51 + 3 * 40, data.begin() + offset + 51 + 3 * 45);

  // 0xB8 0x00: Whether to start current calibration
  // 0xB9 0x00 0x00 0x00 0x00: Actual battery capacity
  //   Firmware version >= 10.10 required
  //   See https://github.com/syssi/esphome-jk-bms/issues/212 for details
  this->actual_battery_capacity_sensor_ = (float) jk_get_32bit(offset + 54 + 3 * 45);

  // 0xBA 0x42 0x54 0x33 0x30 0x37 0x32 0x30 0x32 0x30 0x31 0x32 0x30
  //      0x30 0x30 0x30 0x32 0x30 0x30 0x35 0x32 0x31 0x30 0x30 0x31: Manufacturer ID naming
  this->manufacturer_text_sensor_ =
                       std::string(data.begin() + offset + 59 + 3 * 45, data.begin() + offset + 83 + 3 * 45);

  // 0xC0 0x01: Protocol version number
  this->protocol_version_sensor_ = (float) data[offset + 84 + 3 * 45];

  // 00 00 00 00 68 00 00 54 D1: End of frame

  ESP_LOGI(TAG, "Updated.");
}

void JkBms::update() {
  ESP_LOGI(TAG, "Requesting update.");
  this->track_online_status_();
  this->read_registers(FUNCTION_READ_ALL, ADDRESS_READ_ALL);

  if (this->enable_fake_traffic_) {
    // Start: 0x4E, 0x57, 0x01, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01
    this->on_jk_modbus_data(
        FUNCTION_READ_ALL,
        {
            0x79, 0x2A, 0x01, 0x0E, 0xED, 0x02, 0x0E, 0xFA, 0x03, 0x0E, 0xF7, 0x04, 0x0E, 0xEC, 0x05, 0x0E, 0xF8, 0x06,
            0x0E, 0xFA, 0x07, 0x0E, 0xF1, 0x08, 0x0E, 0xF8, 0x09, 0x0E, 0xE3, 0x0A, 0x0E, 0xFA, 0x0B, 0x0E, 0xF1, 0x0C,
            0x0E, 0xFB, 0x0D, 0x0E, 0xFB, 0x0E, 0x0E, 0xF2, 0x80, 0x00, 0x1D, 0x81, 0x00, 0x1E, 0x82, 0x00, 0x1C, 0x83,
            0x14, 0xEF, 0x84, 0x80, 0xD0, 0x85, 0x0F, 0x86, 0x02, 0x87, 0x00, 0x04, 0x89, 0x00, 0x00, 0x00, 0x00, 0x8A,
            0x00, 0x0E, 0x8B, 0x00, 0x00, 0x8C, 0x00, 0x07, 0x8E, 0x16, 0x26, 0x8F, 0x10, 0xAE, 0x90, 0x0F, 0xD2, 0x91,
            0x0F, 0xA0, 0x92, 0x00, 0x05, 0x93, 0x0B, 0xEA, 0x94, 0x0C, 0x1C, 0x95, 0x00, 0x05, 0x96, 0x01, 0x2C, 0x97,
            0x00, 0x07, 0x98, 0x00, 0x03, 0x99, 0x00, 0x05, 0x9A, 0x00, 0x05, 0x9B, 0x0C, 0xE4, 0x9C, 0x00, 0x08, 0x9D,
            0x01, 0x9E, 0x00, 0x5A, 0x9F, 0x00, 0x46, 0xA0, 0x00, 0x64, 0xA1, 0x00, 0x64, 0xA2, 0x00, 0x14, 0xA3, 0x00,
            0x46, 0xA4, 0x00, 0x46, 0xA5, 0xFF, 0xEC, 0xA6, 0xFF, 0xF6, 0xA7, 0xFF, 0xEC, 0xA8, 0xFF, 0xF6, 0xA9, 0x0E,
            0xAA, 0x00, 0x00, 0x00, 0x0E, 0xAB, 0x01, 0xAC, 0x01, 0xAD, 0x04, 0x11, 0xAE, 0x01, 0xAF, 0x01, 0xB0, 0x00,
            0x0A, 0xB1, 0x14, 0xB2, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x00, 0x00, 0x00, 0x00, 0xB3, 0x00, 0xB4, 0x49,
            0x6E, 0x70, 0x75, 0x74, 0x20, 0x55, 0x73, 0xB5, 0x32, 0x31, 0x30, 0x31, 0xB6, 0x00, 0x00, 0xE2, 0x00, 0xB7,
            0x48, 0x36, 0x2E, 0x58, 0x5F, 0x5F, 0x53, 0x36, 0x2E, 0x31, 0x2E, 0x33, 0x53, 0x5F, 0x5F, 0xB8, 0x00, 0xB9,
            0x00, 0x00, 0x00, 0x00, 0xBA, 0x42, 0x54, 0x33, 0x30, 0x37, 0x32, 0x30, 0x32, 0x30, 0x31, 0x32, 0x30, 0x30,
            0x30, 0x30, 0x32, 0x30, 0x30, 0x35, 0x32, 0x31, 0x30, 0x30, 0x31, 0xC0, 0x01,
        });
    // End: 0x00 0x00 0x00 0x00 0x68 0x00 0x00 0x54 0xD1

    // Start: 0x4E, 0x57, 0x01, 0x18, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01
    /*
    this->on_jk_modbus_data(
        FUNCTION_READ_ALL,
        {
            0x79, 0x27, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x10, 0x34, 0x04, 0x10, 0x28, 0x05, 0x10, 0x29, 0x06,
            0x10, 0x35, 0x07, 0x10, 0x2B, 0x08, 0x10, 0x2B, 0x09, 0x10, 0x35, 0x0A, 0x10, 0x35, 0x0B, 0x10, 0x35, 0x0C,
            0x10, 0x3D, 0x0D, 0x10, 0x26, 0x80, 0x00, 0x1A, 0x81, 0x00, 0x18, 0x82, 0x00, 0x18, 0x83, 0x11, 0xCE, 0x84,
            0x00, 0x00, 0x85, 0x00, 0x86, 0x02, 0x87, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x00, 0x0D, 0x8B,
            0x00, 0x00, 0x8C, 0x00, 0x08, 0x8E, 0x15, 0x54, 0x8F, 0x0E, 0xBA, 0x90, 0x10, 0x68, 0x91, 0x10, 0x04, 0x92,
            0x00, 0x05, 0x93, 0x0B, 0x54, 0x94, 0x0C, 0x80, 0x95, 0x00, 0x05, 0x96, 0x01, 0x2C, 0x97, 0x00, 0x3C, 0x98,
            0x01, 0x2C, 0x99, 0x00, 0x19, 0x9A, 0x00, 0x1E, 0x9B, 0x0C, 0xE4, 0x9C, 0x00, 0x0A, 0x9D, 0x01, 0x9E, 0x00,
            0x5A, 0x9F, 0x00, 0x46, 0xA0, 0x00, 0x64, 0xA1, 0x00, 0x64, 0xA2, 0x00, 0x14, 0xA3, 0x00, 0x46, 0xA4, 0x00,
            0x46, 0xA5, 0xFF, 0xEC, 0xA6, 0xFF, 0xF6, 0xA7, 0xFF, 0xEC, 0xA8, 0xFF, 0xF6, 0xA9, 0x0D, 0xAA, 0x00, 0x00,
            0x00, 0x05, 0xAB, 0x00, 0xAC, 0x00, 0xAD, 0x02, 0xD5, 0xAE, 0x01, 0xAF, 0x01, 0xB0, 0x00, 0x0A, 0xB1, 0x14,
            0xB2, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x00, 0x00, 0x00, 0x00, 0xB3, 0x00, 0xB4, 0x49, 0x6E, 0x70, 0x75,
            0x74, 0x20, 0x55, 0x73, 0xB5, 0x32, 0x31, 0x30, 0x36, 0xB6, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x48, 0x37, 0x2E,
            0x58, 0x5F, 0x5F, 0x53, 0x37, 0x2E, 0x31, 0x2E, 0x30, 0x48, 0x5F, 0x5F, 0xB8, 0x00, 0xB9, 0x00, 0x00, 0x00,
            0x00, 0xBA, 0x42, 0x54, 0x33, 0x30, 0x37, 0x32, 0x30, 0x32, 0x30, 0x31, 0x32, 0x30, 0x30, 0x30, 0x30, 0x32,
            0x30, 0x30, 0x35, 0x32, 0x31, 0x30, 0x30, 0x31, 0xC0, 0x01,
        });
    */
    // End: 0x00 0x00 0x00 0x00 0x68 0x00 0x00 0x47 0x28
  }
}

void JkBms::track_online_status_() {
  if (this->no_response_count_ < MAX_NO_RESPONSE_COUNT) {
    this->no_response_count_++;
  }
  if (this->no_response_count_ == MAX_NO_RESPONSE_COUNT) {
    this->online_status_ = false;
    this->no_response_count_++;
  }
}

void JkBms::reset_online_status_tracker_() {
  this->no_response_count_ = 0;
  this->online_status_ = true;
}

std::string JkBms::error_bits_to_string_(const uint16_t mask) {
  bool first = true;
  std::string errors_list = "";

  if (mask) {
    for (int i = 0; i < ERRORS_SIZE; i++) {
      if (mask & (1 << i)) {
        if (first) {
          first = false;
        } else {
          errors_list.append(";");
        }
        errors_list.append(ERRORS[i]);
      }
    }
  }

  return errors_list;
}

std::string JkBms::mode_bits_to_string_(const uint16_t mask) {
  bool first = true;
  std::string modes_list = "";

  if (mask) {
    for (int i = 0; i < OPERATION_MODES_SIZE; i++) {
      if (mask & (1 << i)) {
        if (first) {
          first = false;
        } else {
          modes_list.append(";");
        }
        modes_list.append(OPERATION_MODES[i]);
      }
    }
  }

  return modes_list;
}

void JkBms::dump_config() {  // NOLINT(google-readability-function-size,readability-function-size)
  ESP_LOGI(TAG, "JkBms:");
  ESP_LOGI(TAG, "  Address: 0x%02X", this->address_);
  ESP_LOGI(TAG, "  Fake traffic enabled: %d", this->enable_fake_traffic_);
  ESP_LOGI(TAG, "Minimum Cell Voltage %f", this->min_cell_voltage_sensor_);
  ESP_LOGI(TAG, "Maximum Cell Voltage %f", this->max_cell_voltage_sensor_);
  ESP_LOGI(TAG, "Minimum Voltage Cell %f", this->min_voltage_cell_sensor_);
  ESP_LOGI(TAG, "Maximum Voltage Cell %f", this->max_voltage_cell_sensor_);
  ESP_LOGI(TAG, "Delta Cell Voltage %f", this->delta_cell_voltage_sensor_);
  ESP_LOGI(TAG, "Average Cell Voltage %f", this->average_cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 1 %f", this->cells_[0].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 2 %f", this->cells_[1].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 3 %f", this->cells_[2].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 4 %f", this->cells_[3].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 5 %f", this->cells_[4].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 6 %f", this->cells_[5].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 7 %f", this->cells_[6].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 8 %f", this->cells_[7].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 9 %f", this->cells_[8].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 10 %f", this->cells_[9].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 11 %f", this->cells_[10].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 12 %f", this->cells_[11].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 13 %f", this->cells_[12].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 14 %f", this->cells_[13].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 15 %f", this->cells_[14].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 16 %f", this->cells_[15].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 17 %f", this->cells_[16].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 18 %f", this->cells_[17].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 19 %f", this->cells_[18].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 20 %f", this->cells_[19].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 21 %f", this->cells_[20].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 22 %f", this->cells_[21].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 23 %f", this->cells_[22].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Cell Voltage 24 %f", this->cells_[23].cell_voltage_sensor_);
  ESP_LOGI(TAG, "Power Tube Temperature %f", this->power_tube_temperature_sensor_);
  ESP_LOGI(TAG, "Temperature Sensor 1 %f", this->temperature_sensor_1_sensor_);
  ESP_LOGI(TAG, "Temperature Sensor 2 %f", this->temperature_sensor_2_sensor_);
  ESP_LOGI(TAG, "Total Voltage %f", this->total_voltage_sensor_);
  ESP_LOGI(TAG, "Current %f", this->current_sensor_);
  ESP_LOGI(TAG, "Power %f", this->power_sensor_);
  ESP_LOGI(TAG, "Charging Power %f", this->charging_power_sensor_);
  ESP_LOGI(TAG, "Discharging Power %f", this->discharging_power_sensor_);
  ESP_LOGI(TAG, "Capacity Remaining %f", this->capacity_remaining_sensor_);
  ESP_LOGI(TAG, "Capacity Remaining Derived %f", this->capacity_remaining_derived_sensor_);
  ESP_LOGI(TAG, "Temperature Sensors %d", this->temperature_sensors_sensor_);
  ESP_LOGI(TAG, "Charging Cycles %f", this->charging_cycles_sensor_);
  ESP_LOGI(TAG, "Total Charging Cycle Capacity %f", this->total_charging_cycle_capacity_sensor_);
  ESP_LOGI(TAG, "Battery Strings %f", this->battery_strings_sensor_);
  ESP_LOGI(TAG, "Errors Bitmask %02x", this->errors_bitmask_sensor_);
  ESP_LOGI(TAG, "Operation Mode Bitmask %02x", this->operation_mode_bitmask_sensor_);
  ESP_LOGI(TAG, "Total Voltage Overvoltage Protection %f", this->total_voltage_overvoltage_protection_sensor_);
  ESP_LOGI(TAG, "Total Voltage Undervoltage Protection %f", this->total_voltage_undervoltage_protection_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Overvoltage Protection %f", this->cell_voltage_overvoltage_protection_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Overvoltage Recovery %f", this->cell_voltage_overvoltage_recovery_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Overvoltage Delay %f", this->cell_voltage_overvoltage_delay_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Undervoltage Protection %f", this->cell_voltage_undervoltage_protection_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Undervoltage Recovery %f", this->cell_voltage_undervoltage_recovery_sensor_);
  ESP_LOGI(TAG, "Cell Voltage Undervoltage  %f", this->cell_voltage_undervoltage_delay_sensor_);
  ESP_LOGI(TAG, "Cell Pressure Difference Protection %f", this->cell_pressure_difference_protection_sensor_);
  ESP_LOGI(TAG, "Discharging Overcurrent Protection %f", this->discharging_overcurrent_protection_sensor_);
  ESP_LOGI(TAG, "Discharging Overcurrent Delay %f", this->discharging_overcurrent_delay_sensor_);
  ESP_LOGI(TAG, "Charging Overcurrent Protection %f", this->charging_overcurrent_protection_sensor_);
  ESP_LOGI(TAG, "Charging Overcurrent Delay %f", this->charging_overcurrent_delay_sensor_);
  ESP_LOGI(TAG, "Balance Starting Voltage %f", this->balance_starting_voltage_sensor_);
  ESP_LOGI(TAG, "Balance Opening Pressure Difference %f", this->balance_opening_pressure_difference_sensor_);
  ESP_LOGI(TAG, "Power Tube Temperature Protection %f", this->power_tube_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Power Tube Temperature Recovery %f", this->power_tube_temperature_recovery_sensor_);
  ESP_LOGI(TAG, "Temperature Sensor Temperature Protection %f", this->temperature_sensor_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Temperature Sensor Temperature Recovery %f", this->temperature_sensor_temperature_recovery_sensor_);
  ESP_LOGI(TAG, "Temperature Sensor Temperature Difference Protection %f",
             this->temperature_sensor_temperature_difference_protection_sensor_);
  ESP_LOGI(TAG, "Charging High Temperature Protection %f", this->charging_high_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Discharging High Temperature Protection %f", this->discharging_high_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Charging Low Temperature Protection %f", this->charging_low_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Charging Low Temperature Recovery %f", this->charging_low_temperature_recovery_sensor_);
  ESP_LOGI(TAG, "Discharging Low Temperature Protection %f", this->discharging_low_temperature_protection_sensor_);
  ESP_LOGI(TAG, "Discharging Low Temperature Recovery %f", this->discharging_low_temperature_recovery_sensor_);
  ESP_LOGI(TAG, "Total Battery Capacity Setting %f", this->total_battery_capacity_setting_sensor_);
  ESP_LOGI(TAG, "Current Calibration %f", this->current_calibration_sensor_);
  ESP_LOGI(TAG, "Device Address %f", this->device_address_sensor_);
  ESP_LOGI(TAG, "Battery Type %s", this->battery_type_text_sensor_.c_str());
  ESP_LOGI(TAG, "Sleep Wait Time %f", this->sleep_wait_time_sensor_);
  ESP_LOGI(TAG, "Alarm Low Volume %f", this->alarm_low_volume_sensor_);
  ESP_LOGI(TAG, "Password %s", this->password_text_sensor_.c_str());
  ESP_LOGI(TAG, "Device Type %s", this->device_type_text_sensor_.c_str());
  //ESP_LOGI(TAG, "Manufacturing Date", this->manufacturing_date_sensor_);
  ESP_LOGI(TAG, "Total Runtime %f", this->total_runtime_sensor_);
  ESP_LOGI(TAG, "Software Version %s", this->software_version_text_sensor_.c_str());
  // ESP_LOGI(TAG, "Start Current Calibration", this->start_current_calibration_sensor_);
  ESP_LOGI(TAG, "Manufacturer %s", this->manufacturer_text_sensor_.c_str());
  ESP_LOGI(TAG, "Protocol Version %f", this->protocol_version_sensor_);
  ESP_LOGI(TAG, "Balancing %d", this->balancing_binary_sensor_);
  ESP_LOGI(TAG, "Balancing Switch %d", this->balancing_switch_binary_sensor_);
  ESP_LOGI(TAG, "Charging %d", this->charging_binary_sensor_);
  ESP_LOGI(TAG, "Charging Switch %d", this->charging_switch_binary_sensor_);
  ESP_LOGI(TAG, "Discharging %d", this->discharging_binary_sensor_);
  ESP_LOGI(TAG, "Discharging Switch %d", this->discharging_switch_binary_sensor_);
  ESP_LOGI(TAG, "Dedicated Charger Switch %d", this->dedicated_charger_switch_binary_sensor_);
  ESP_LOGI(TAG, "Total Runtime Formatted %s", this->total_runtime_formatted_text_sensor_.c_str());
}

 uint8_t *NotImplemented2Bytes(){
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = 0;
    return reply;
 }

 uint8_t *NotImplemented4Bytes(){
    auto reply = new uint8_t[4];
    reply[0] = 0;
    reply[1] = 0;
    reply[2] = 0;
    reply[3] = 0;
    return reply;
 }

  // Version information
  uint8_t *JkBms::getBMSFirmwareVersion() {
    return NotImplemented4Bytes();
  }
  uint8_t *JkBms::getBMSHardwareVersion() {
    return NotImplemented4Bytes();
  }
  // BMS general status
  uint8_t *JkBms::getNumberOfCells() {
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = cell_count_; // 8 cells
    ESP_LOGI(TAG, "Sending number of cells: %d", cell_count_);
    return reply;
  }
  uint8_t *JkBms::getCellVoltageOrNull(size_t cellNumber) {
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = 0;
    if (cellNumber <= cell_count_){
      ESP_LOGI(TAG, "Sending voltage for cellNumber %d: %f", cellNumber, cells_[cellNumber-1].cell_voltage_sensor_);
      float cellVoltageAdjusted =cells_[cellNumber-1].cell_voltage_sensor_ * 10;
      reply[1] =  static_cast<uint8_t>(cellVoltageAdjusted);
    }
      
    return reply;
  }
  uint8_t *JkBms::getNumberOfTemperatureSensors() {
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = temperature_sensors_sensor_;
    ESP_LOGI(TAG, "Sending number of temperature sensors: %d", cell_count_);
    return reply;
  };
  uint8_t *JkBms::getTemperatureOfSensorOrNull(size_t temperatureSensorNumber) { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = 0;
    if (temperatureSensorNumber <= temperature_sensors_sensor_){
      ESP_LOGI(TAG, "Sending temperature for sensorNumber %d: %f", temperatureSensorNumber, temperature_sensors_[temperatureSensorNumber-1].temperature_sensor_);
      float temperatureAdjusted = (temperature_sensors_[temperatureSensorNumber-1].temperature_sensor_ + 273.15) * 10;
      uint16_t tempKelvin =  static_cast<uint16_t>(temperatureAdjusted);

      reply[0] = (tempKelvin >> 8) & 0xFF;
      reply[1] = tempKelvin & 0xFF;
    }
      
    return reply;
  };
  uint8_t *JkBms::getModuleChargeCurrent() { 
    auto reply = new uint8_t[2];

    float chargingCurrent = this->charging_current_sensor_ * 10;
    uint16_t chargingCurrentAdjusted = static_cast<uint16_t>(chargingCurrent);
    reply[0] = (chargingCurrentAdjusted >> 8) & 0xFF;
    reply[1] = chargingCurrentAdjusted & 0xFF;

    ESP_LOGI(TAG, "Sending charging current: %f", chargingCurrent);
    return reply;
  };
  uint8_t *JkBms::getModuleDischargeCurrent() { 
    auto reply = new uint8_t[2];

    float dischargingCurrent = this->discharging_current_sensor_ * 10;
    uint16_t dischargingCurrentAdjusted = static_cast<uint16_t>(dischargingCurrent);
    reply[0] = (dischargingCurrentAdjusted >> 8) & 0xFF;
    reply[1] = dischargingCurrentAdjusted & 0xFF;

    ESP_LOGI(TAG, "Sending discharge current: %f", dischargingCurrent);
    return reply;
  };
  uint8_t *JkBms::getModuleVoltage() { 
    auto reply = new uint8_t[2];

    float totalVoltage = this->total_voltage_sensor_ * 10;
    uint16_t totalVoltageAdjusted = static_cast<uint16_t>(totalVoltage);
    reply[0] = (totalVoltageAdjusted >> 8) & 0xFF;
    reply[1] = totalVoltageAdjusted & 0xFF;

    ESP_LOGI(TAG, "Sending total voltage: %d", totalVoltageAdjusted);
    return reply;
  };
  uint8_t *JkBms::getStateOfCharge() { 
    auto reply = new uint8_t[2];

    float capacityRemaining = this->capacity_remaining_sensor_;
    uint16_t capacityRemainingAdjusted = static_cast<uint16_t>(capacityRemaining);
    reply[0] = (capacityRemainingAdjusted >> 8) & 0xFF;
    reply[1] = capacityRemainingAdjusted & 0xFF;

    ESP_LOGI(TAG, "Sending capacity remaining: %f", capacityRemaining);
    return reply;
  };
  uint8_t *JkBms::getModuleTotalCapacity() { 
    auto reply = new uint8_t[4];

    float totalCapacityMilliAhAdjustedFloat = this->total_battery_capacity_setting_sensor_ * 1000;
    uint32_t totalCapacityMilliAhAdjusted = static_cast<uint32_t>(totalCapacityMilliAhAdjustedFloat);
    reply[0] = (totalCapacityMilliAhAdjusted >> 24) & 0xFF;
    reply[1] = (totalCapacityMilliAhAdjusted >> 16) & 0xFF;
    reply[2] = (totalCapacityMilliAhAdjusted >> 8) & 0xFF;
    reply[3] = totalCapacityMilliAhAdjusted & 0xFF;

    ESP_LOGI(TAG, "Sending total cxapacity: %lu", totalCapacityMilliAhAdjusted);
    return reply;
  };

  // BMS warning information inquiry
  // All reply with 2 bytes and only the LSB is set to one of:
  //      0x00 - Normal
  //      0x01 - Below normal
  //      0x02 - Above higher limit
  //      0xF0 - Other error
  const uint8_t LibProtocolState_Normal = 0x00;
  const uint8_t LibProtocolState_BelowNormal = 0x01;
  const uint8_t LibProtocolState_AboveHigherLimit = 0x02;
  const uint8_t LibProtocolState_OtherError = 0xF0;

  uint8_t *JkBms::getNumberOfCellsForWarningInfo() { 
    return NotImplemented2Bytes();
  };
  uint8_t *JkBms::getCellPairVoltageState(size_t oddCellNumber) { 
    return NotImplemented2Bytes();
  };
  uint8_t *JkBms::getNumberOfTemperatureSensorsForWarningInfo() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = temperature_sensors_sensor_;
    ESP_LOGI(TAG, "Sending number of temperature sensors for warning info: %d", cell_count_);
    return reply;
  };
  uint8_t *JkBms::getTemperatureSensorPairState(size_t oddTemperatureSensorNumber) { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = 0;

    return reply;
  };
  uint8_t *JkBms::getModuleChargeVoltageState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_charging_overvoltage) == errors_bitmask_alarm_charging_overvoltage)
      reply[1] |= LibProtocolState_AboveHigherLimit;

    return reply;
  };
  uint8_t *JkBms::getModuleDischargeVoltageState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_discharging_undervoltage) == errors_bitmask_alarm_discharging_undervoltage)
      reply[1] |= LibProtocolState_BelowNormal;
    
    return reply;
  };
  uint8_t *JkBms::getCellChargeVoltageState() { 
    return NotImplemented2Bytes();
  };
  uint8_t *JkBms::getCellDischargeVoltageState() { 
    return NotImplemented2Bytes();
  };
  uint8_t *JkBms::getModuleChargeCurrentState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_charging_overcurrent) == errors_bitmask_alarm_charging_overcurrent)
      reply[1] |= LibProtocolState_AboveHigherLimit;

    return reply;
  };
  uint8_t *JkBms::getModuleDischargeCurrentState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_discharging_overcurrent) == errors_bitmask_alarm_discharging_overcurrent)
      reply[1] |= LibProtocolState_AboveHigherLimit;

    return reply;
  };
  uint8_t *JkBms::getModuleChargeTemperatureState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_battery_low_temperature) == errors_bitmask_alarm_battery_low_temperature)
    {
      reply[1] |= LibProtocolState_BelowNormal;
      return reply;
    }

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_battery_over_temperature) == errors_bitmask_alarm_battery_over_temperature)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_battery_box_overtemperature) == errors_bitmask_alarm_battery_box_overtemperature)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_power_tube_over_temp) == errors_bitmask_alarm_power_tube_over_temp)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    return reply;
  };
  uint8_t *JkBms::getModuleDischargeTemperatureState() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = LibProtocolState_Normal;

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_battery_over_temperature) == errors_bitmask_alarm_battery_over_temperature)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_battery_box_overtemperature) == errors_bitmask_alarm_battery_box_overtemperature)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    if ((errors_bitmask_sensor_ & errors_bitmask_alarm_power_tube_over_temp) == errors_bitmask_alarm_power_tube_over_temp)
    {
      reply[1] |= LibProtocolState_AboveHigherLimit;
      return reply;
    }

    return reply;
  };
  uint8_t *JkBms::getCellChargeTemperatureState() { 
    return NotImplemented2Bytes();
  };
  uint8_t *JkBms::getCellDischargeTemperatureState() { 
    return NotImplemented2Bytes();
  };

  // BMS charge and discharge information inquiry
  uint8_t *JkBms::getChargeVoltageLimit() { 
    auto reply = new uint8_t[2];
    float chargingVoltageLimit = this->cell_voltage_overvoltage_recovery_sensor_ * this->cell_count_ * 10;
    uint16_t chargingVoltageLimitInt = static_cast<uint16_t>(chargingVoltageLimit);
    reply[0] = (chargingVoltageLimitInt >> 8) & 0xFF;
    reply[1] = chargingVoltageLimitInt & 0xFF;

    ESP_LOGI(TAG, "Sending charge voltage limit: %d", chargingVoltageLimitInt);
    return reply;
  };
  uint8_t *JkBms::getDischargeVoltageLimit() { 
    auto reply = new uint8_t[2];
    float dischargeVoltageLimit = this->cell_voltage_undervoltage_recovery_sensor_ * this->cell_count_ * 10;
    uint16_t dischargeVoltageLimitInt = static_cast<uint16_t>(dischargeVoltageLimit);
    reply[0] = (dischargeVoltageLimitInt >> 8) & 0xFF;
    reply[1] = dischargeVoltageLimitInt & 0xFF;

    ESP_LOGI(TAG, "Sending discharge voltage limit: %d", dischargeVoltageLimitInt);
    return reply;
  };
  uint8_t *JkBms::getChargeCurrentLimit() { 
    auto reply = new uint8_t[2];
    float chargingCurrentLimit = this->charging_overcurrent_protection_sensor_ * 10;
    uint16_t chargingCurrentLimitInt = static_cast<uint16_t>(chargingCurrentLimit);
    reply[0] = (chargingCurrentLimitInt >> 8) & 0xFF;
    reply[1] = chargingCurrentLimitInt & 0xFF;

    ESP_LOGI(TAG, "Sending charging current limit: %d", chargingCurrentLimitInt);
    return reply;
  };
  uint8_t *JkBms::getDischargeCurrentLimit() { 
    auto reply = new uint8_t[2];
    float dischargeCurrentLimit = this->discharging_overcurrent_protection_sensor_ * 10;
    uint16_t dischargeCurrentLimitInt = static_cast<uint16_t>(dischargeCurrentLimit);
    reply[0] = (dischargeCurrentLimitInt >> 8) & 0xFF;
    reply[1] = dischargeCurrentLimitInt & 0xFF;

    ESP_LOGI(TAG, "Sending discharge current limit: %d", dischargeCurrentLimitInt);
    return reply;
  };


  uint8_t *JkBms::getChargeDischargeStatus() { 
    auto reply = new uint8_t[2];
    reply[0] = 0;
    reply[1] = 0;

    const uint8_t fullChargeRequest = 8;   // 0000 1000 Set when BMS needs battery fully charged
    const uint8_t chargeImmediately2 = 16; // 0001 0000 Set when SoC is low, like 10~14%
    const uint8_t chargeImmediately = 32;  // 0010 0000 Set when SoC is very low, like 5~9%
    const uint8_t dischargeEnable = 64;    // 0100 0000
    const uint8_t chargeEnable = 128;      // 1000 0000

    if (this->charging_binary_sensor_ && this->temperature_sensor_1_sensor_ < 35 && this ->temperature_sensor_2_sensor_ < 35)
      reply[1] |= chargeEnable;
    
    if (this->discharging_binary_sensor_ && this->temperature_sensor_1_sensor_ < 35 && this -> temperature_sensor_2_sensor_ < 35)
      reply[1] |= dischargeEnable;

    if (this->capacity_remaining_sensor_ >= 10 && this->capacity_remaining_sensor_ <=15)
      reply[1] |= chargeImmediately2;

    if (this->capacity_remaining_sensor_ < 10)
      reply[1] |= chargeImmediately;

    // no idea when to request full charge

    return reply;

  };
  uint8_t *JkBms::getRuntimeToEmptySeconds() { 
    return NotImplemented2Bytes();
  };

}  // namespace jk_bms
}  // namespace esphome