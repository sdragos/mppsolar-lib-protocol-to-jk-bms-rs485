#pragma once

#include "bms_lib_protocol_data_adapter.h"
#include "esphome/components/jk_modbus/jk_modbus.h"

namespace esphome {
namespace jk_bms {

class JkBms: public jk_modbus::JkModbusDevice, public sdragos::mppsolar::BMSLibProtocolDataAdapter {
 private:
  bool has_data_ = false;
 public:

  void set_enable_fake_traffic(bool enable_fake_traffic) { enable_fake_traffic_ = enable_fake_traffic; }

  void dump_config();

  void on_jk_modbus_data(const uint8_t &function, const std::vector<uint8_t> &data) override;

  void update();

  // Begin BMSLibProtocolDataAdapter overrides

  bool hasData() override { return has_data_; };

  // Version information
  uint8_t *getBMSFirmwareVersion() override;
  uint8_t *getBMSHardwareVersion() override;

  // BMS general status
  uint8_t *getNumberOfCells() override;
  uint8_t *getCellVoltageOrNull(size_t cellNumber) override;
  uint8_t *getNumberOfTemperatureSensors() override;
  uint8_t *getTemperatureOfSensorOrNull(size_t temperatureSensorNumber) override;
  uint8_t *getModuleChargeCurrent() override;
  uint8_t *getModuleDischargeCurrent() override;
  uint8_t *getModuleVoltage() override;
  uint8_t *getStateOfCharge() override;
  uint8_t *getModuleTotalCapacity() override;

  // BMS warning information inquiry
  // All reply with 2 bytes and only the LSB is set to one of:
  //      0x00 - Normal
  //      0x01 - Below normal
  //      0x02 - Above higher limit
  //      0xF0 - Other error
  uint8_t *getNumberOfCellsForWarningInfo() override;
  uint8_t *getCellPairVoltageState(size_t oddCellNumber) override;
  uint8_t *getNumberOfTemperatureSensorsForWarningInfo() override;
  uint8_t *getTemperatureSensorPairState(size_t oddTemperatureSensorNumber) override;
  uint8_t *getModuleChargeVoltageState() override;
  uint8_t *getModuleDischargeVoltageState() override;
  uint8_t *getCellChargeVoltageState() override;
  uint8_t *getCellDischargeVoltageState() override;
  uint8_t *getModuleChargeCurrentState() override;
  uint8_t *getModuleDischargeCurrentState() override;
  uint8_t *getModuleChargeTemperatureState() override;
  uint8_t *getModuleDischargeTemperatureState() override;
  uint8_t *getCellChargeTemperatureState() override;
  uint8_t *getCellDischargeTemperatureState() override;

  // BMS charge and discharge information inquiry
  uint8_t *getChargeVoltageLimit() override;
  uint8_t *getDischargeVoltageLimit() override;
  uint8_t *getChargeCurrentLimit() override;
  uint8_t *getDischargeCurrentLimit() override;
  uint8_t *getChargeDischargeStatus() override;
  uint8_t *getRuntimeToEmptySeconds() override;

  // End BMSLibProtocolDataAdapter overrides

 protected:
  float min_cell_voltage_sensor_;
  float max_cell_voltage_sensor_;
  float min_voltage_cell_sensor_;
  float max_voltage_cell_sensor_;
  float delta_cell_voltage_sensor_;
  float average_cell_voltage_sensor_;

  float power_tube_temperature_sensor_;
  float temperature_sensor_1_sensor_;
  float temperature_sensor_2_sensor_;

  float total_voltage_sensor_;

  float current_sensor_;
  float power_sensor_;
  float charging_power_sensor_;
  float discharging_power_sensor_;
  
  float capacity_remaining_sensor_;
  float capacity_remaining_derived_sensor_;
  
  float temperature_sensors_sensor_;

  float charging_cycles_sensor_;
  float total_charging_cycle_capacity_sensor_;
  float battery_strings_sensor_;
  
  uint16_t errors_bitmask_sensor_;
  uint16_t operation_mode_bitmask_sensor_;
  float total_voltage_overvoltage_protection_sensor_;
  float total_voltage_undervoltage_protection_sensor_;
  float cell_voltage_overvoltage_protection_sensor_;
  float cell_voltage_overvoltage_recovery_sensor_;
  float cell_voltage_overvoltage_delay_sensor_;
  float cell_voltage_undervoltage_protection_sensor_;
  float cell_voltage_undervoltage_recovery_sensor_;
  float cell_voltage_undervoltage_delay_sensor_;
  float cell_pressure_difference_protection_sensor_;
  float discharging_overcurrent_protection_sensor_;
  float discharging_overcurrent_delay_sensor_;
  float charging_overcurrent_protection_sensor_;
  float charging_overcurrent_delay_sensor_;
  float balance_starting_voltage_sensor_;
  float balance_opening_pressure_difference_sensor_;
  float power_tube_temperature_protection_sensor_;
  float power_tube_temperature_recovery_sensor_;
  float temperature_sensor_temperature_protection_sensor_;
  float temperature_sensor_temperature_recovery_sensor_;
  float temperature_sensor_temperature_difference_protection_sensor_;
  float charging_high_temperature_protection_sensor_;
  float discharging_high_temperature_protection_sensor_;
  float charging_low_temperature_protection_sensor_;
  float charging_low_temperature_recovery_sensor_;
  float discharging_low_temperature_protection_sensor_;
  float discharging_low_temperature_recovery_sensor_;

  float total_battery_capacity_setting_sensor_;
  float charging_sensor_;
  float discharging_sensor_;
  float current_calibration_sensor_;
  float device_address_sensor_;
  float sleep_wait_time_sensor_;
  float alarm_low_volume_sensor_;
  //sensor::Sensor *password_sensor_;
  //sensor::Sensor *manufacturing_date_sensor_;
  float total_runtime_sensor_;
  //sensor::Sensor *start_current_calibration_sensor_;
  float actual_battery_capacity_sensor_;
  float protocol_version_sensor_;

  bool balancing_binary_sensor_;
  bool balancing_switch_binary_sensor_;
  bool charging_binary_sensor_;
  bool charging_switch_binary_sensor_;
  bool discharging_binary_sensor_;
  bool discharging_switch_binary_sensor_;
  bool dedicated_charger_switch_binary_sensor_;
  bool online_status_ = false;

  std::string errors_text_sensor_;
  std::string operation_mode_text_sensor_;
  std::string battery_type_text_sensor_;
  std::string password_text_sensor_;
  std::string device_type_text_sensor_;
  std::string software_version_text_sensor_;
  std::string manufacturer_text_sensor_;
  std::string total_runtime_formatted_text_sensor_;

  uint8_t cell_count_;

  struct Cell {
    float cell_voltage_sensor_;
  } cells_[24];

  bool enable_fake_traffic_;
  uint8_t no_response_count_{0};

  void on_status_data_(const std::vector<uint8_t> &data);
  void reset_online_status_tracker_();
  void track_online_status_();

  std::string error_bits_to_string_(uint16_t bitmask);
  std::string mode_bits_to_string_(uint16_t bitmask);

  float get_temperature_(const uint16_t value) {
    if (value > 100)
      return (float) (100 - (int16_t) value);

    return (float) value;
  };

  float get_current_(const uint16_t value, const uint8_t protocol_version) {
    float current = 0.0f;
    if (protocol_version == 0x01) {
      if ((value & 0x8000) == 0x8000) {
        current = (float) (value & 0x7FFF);
      } else {
        current = (float) (value & 0x7FFF) * -1;
      }
    }

    return current;
  };

  std::string format_total_runtime_(const uint32_t value) {
    int seconds = (int) value;
    int years = seconds / (24 * 3600 * 365);
    seconds = seconds % (24 * 3600 * 365);
    int days = seconds / (24 * 3600);
    seconds = seconds % (24 * 3600);
    int hours = seconds / 3600;
    return (years ? std::__cxx11::to_string(years) + "y " : "") + (days ? std::__cxx11::to_string(days) + "d " : "") +
           (hours ? std::__cxx11::to_string(hours) + "h" : "");
  }

  bool check_bit_(uint16_t mask, uint16_t flag) { return (mask & flag) == flag; }
};

}  // namespace jk_bms
}  // namespace esphome