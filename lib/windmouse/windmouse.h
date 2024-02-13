#include <vector>
#include <Arduino.h>
#include "USBHIDMouse.h"
#include <ramdb.h>
#include <map>

#define HWMouse USBHIDMouse

#define cooint_t int16_t

class WindMouse {
    public:
        WindMouse();
        ~WindMouse();
        void move(cooint_t x, cooint_t y);
        void press(uint8_t b = MOUSE_LEFT);
        void release(uint8_t b = MOUSE_LEFT);
        bool isPressed(uint8_t b = MOUSE_LEFT);
        void syncParams();
        void pullAndMove();

    private:

        HWMouse* m_mouse = nullptr;
        std::pair<double, double> errs;
        // WindMouse algorithm.
        // Released under the terms of the GPLv3 license.
        // G_0 - magnitude of the gravitational fornce
        // W_0 - magnitude of the wind force fluctuations
        // M_0 - maximum step size (velocity clip threshold)
        // D_0 - distance where wind behavior changes from random to damped
        int8_t G, W, M, D;
        int16_t report_rate;
        double final_accuracy;

        Queue* m_queue = nullptr;

        void windMouse(
            cooint_t x, cooint_t y,
            double final_accuracy
        );
        void do_move(cooint_t x, cooint_t y);
        void do_syncParam(const std::string& key, int8_t& param);
        void do_syncParam(const std::string& key, int16_t& param);
        void do_syncParam(const std::string& key, double& param);

};

