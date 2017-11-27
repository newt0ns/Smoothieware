/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include "libs/Module.h"
#include "libs/Kernel.h"
#include "TemperatureAutoReport.h"
#include "PublicDataRequest.h"
#include "PublicData.h"
#include "StreamOutputPool.h"
#include "Config.h"
#include "ConfigValue.h"
#include "checksumm.h"
#include "Gcode.h"
#include "TemperatureControlPublicAccess.h"

#define enable_checksum                              CHECKSUM("enable")
#define auto_report_checksum                         CHECKSUM("auto_report")
#define auto_report_rate_checksum                    CHECKSUM("report_rate")
#define get_m_code_checksum                          CHECKSUM("get_m_code")


TemperatureAutoReport::TemperatureAutoReport()
{
      this->send_report = false;
      this->enabled = false;
      this->report_rate = 5;
      this->last_report_timer = 0;
}

TemperatureAutoReport::~TemperatureAutoReport()
{
      THEKERNEL->unregister_for_event(ON_GCODE_RECEIVED, this);
      THEKERNEL->unregister_for_event(ON_SECOND_TICK, this);
      THEKERNEL->unregister_for_event(ON_IDLE, this);
}

void TemperatureAutoReport::on_module_loaded()
{
    //Read config options
    this->get_m_code          = THEKERNEL->config->value(temperature_control_checksum, auto_report_checksum, get_m_code_checksum)->by_default(105)->as_number();
    this->report_rate         = THEKERNEL->config->value(temperature_control_checksum, auto_report_checksum, auto_report_rate_checksum)->by_default(1)->as_number();
    this->enabled             = THEKERNEL->config->value(temperature_control_checksum, auto_report_checksum, enable_checksum)->by_default(false)->as_bool();

    // Register for events
    this->register_for_event(ON_GCODE_RECEIVED);
    this->register_for_event(ON_SECOND_TICK);
    this->register_for_event(ON_IDLE);
}

void TemperatureAutoReport::on_gcode_received(void *argument)
{
      Gcode *gcode = static_cast<Gcode *>(argument);
      if (gcode->has_m) {
            if( gcode->m == this->get_m_code ) {
                  this->last_report_timer = 0; //Reset our timer since we polled for a temperature status already
            }
            else if(gcode->m == 155) {
                  if(gcode->has_letter('S')) { //Alter reporting behavior
                        this->enabled = gcode->get_uint('S')>0;
                  }
            }
            else if(gcode->m == 115) {
                  gcode->txt_after_ok.append("\nCap:AUTOREPORT_TEMP:1");
            }
      }
}

void TemperatureAutoReport::on_idle(void *argument)
{
      if(this->send_report) {
            this->send_report = false;
            this->last_report_timer = 0;

            if(this->enabled) {
                  std::vector<struct pad_temperature> controllers;
            
                  bool ok = PublicData::get_value(temperature_control_checksum, poll_controls_checksum, &controllers);
                  if (ok) {
                        for (auto &c : controllers) {
                              THEKERNEL->streams->printf("%s:%3.1f /%3.1f @%d ", c.designator.c_str(), c.current_temperature, ((c.target_temperature <= 0) ? 0.0 : c.target_temperature), c.pwm);
                        }      
                        THEKERNEL->streams->printf("\n");
                  }
            }
      }
}


void TemperatureAutoReport::on_second_tick(void *argument)
{
      this->last_report_timer++;
      if(this->last_report_timer>this->report_rate)
      {
            this->send_report = true;
      }
}