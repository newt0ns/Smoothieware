/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include "AD8495.h"
#include "libs/Kernel.h"
#include "libs/Pin.h"
#include "Config.h"
#include "checksumm.h"
#include "Adc.h"
#include "ConfigValue.h"
#include "libs/Median.h"
#include "utils.h"
#include "StreamOutputPool.h"

#include <fastmath.h>

#include "MRI_Hooks.h"

#define UNDEFINED -1

#define AD8495_pin_checksum            CHECKSUM("ad8495_pin")
#define AD8495_offset_checksum         CHECKSUM("ad8495_offset")
#define AD8495_alpha_checksum          CHECKSUM("ad8495_alpha")

AD8495::AD8495()
{
    min_temp= 999;
    max_temp= 0;
    this->smoothed_temperature = infinityf();
}

AD8495::~AD8495()
{
}

// Get configuration from the config file
void AD8495::UpdateConfig(uint16_t module_checksum, uint16_t name_checksum)
{
    // Thermistor pin for ADC readings
    this->AD8495_pin.from_string(THEKERNEL->config->value(module_checksum, name_checksum, AD8495_pin_checksum)->required()->as_string());
    this->AD8495_offset = THEKERNEL->config->value(module_checksum, name_checksum, AD8495_offset_checksum)->by_default(0)->as_number(); // Stated offset. For Adafruit board it is 250C. If pin 2(REF) of amplifier is connected to 0V then there is 0C offset.
	this->alpha = THEKERNEL->config->value(module_checksum, name_checksum, AD8495_alpha_checksum)->by_default(1)->as_number();
    THEKERNEL->adc->enable_pin(&AD8495_pin);
}


float AD8495::get_temperature()
{
    float raw_temperature= adc_value_to_temperature(new_AD8495_reading());
    

    if(!isinf(raw_temperature)) {

        if(isinf(this->smoothed_temperature)) //Re-establishing a valid temperature history, reset the filter
        {
            this->smoothed_temperature = raw_temperature;
        }
        else 
        {
            if(raw_temperature > this->smoothed_temperature)
            {
                this->smoothed_temperature = this->smoothed_temperature+((raw_temperature - this->smoothed_temperature)/this->alpha);
            }
            else
            {
                this->smoothed_temperature = this->smoothed_temperature-((this->smoothed_temperature - raw_temperature)/this->alpha);
            }
        }

        // keep track of min/max for M305
        if(this->smoothed_temperature > this->max_temp) this->max_temp= this->smoothed_temperature;
        if(this->smoothed_temperature < this->min_temp) this->min_temp= this->smoothed_temperature;
     
        return this->smoothed_temperature;   
    }
        this->smoothed_temperature = infinityf();
        return infinityf();
}

void AD8495::get_raw()
{

    int adc_value= new_AD8495_reading();
    const uint32_t max_adc_value= THEKERNEL->adc->get_max_value();
    float t=((float)adc_value)/(((float)max_adc_value)/3.3*0.005);

    t = t - this->AD8495_offset;
	
    THEKERNEL->streams->printf("adc= %d, max_adc= %lu, temp= %f, offset = %f\n", adc_value,max_adc_value,t, this->AD8495_offset);

    // reset the min/max
    min_temp= max_temp= t;
}

float AD8495::adc_value_to_temperature(uint32_t adc_value)
{
    const uint32_t max_adc_value= THEKERNEL->adc->get_max_value();
    if ((adc_value >= max_adc_value))
        return infinityf();

    float t=((float)adc_value)/(((float)max_adc_value)/3.3*0.005);

    t=t-this->AD8495_offset;
	
    return t;
}

int AD8495::new_AD8495_reading()
{
    // filtering now done in ADC
    return THEKERNEL->adc->read(&AD8495_pin);
}
