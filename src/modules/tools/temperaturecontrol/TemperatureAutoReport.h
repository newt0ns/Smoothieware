#ifndef TEMPERATUREAUTOREPORT_H
#define TEMPERATUREAUTOREPORT_H

#include "Module.h"

class TemperatureAutoReport : public Module {
    public:
        TemperatureAutoReport();
        ~TemperatureAutoReport();

        void on_module_loaded();
        void on_gcode_received(void *);        
        void on_idle(void *);
        void on_second_tick(void *);

    private:
        struct {
            bool enabled:1;
            bool send_report:1;            
            uint16_t get_m_code:10;
            uint8_t report_rate;
            uint8_t last_report_timer;

        };
};





#endif
