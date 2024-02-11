#include <vector>
#include <Arduino.h>
#include "USBHIDMouse.h"

#define HWMouse USBHIDMouse

#define cooint_t int16_t
#define REPORT_RATE 100
#define T_RAND_CLIP 0.5
#define FINAL_ACCURACY 200.0

class WindMouse {
    public:
        WindMouse();
        ~WindMouse();
        void move(cooint_t x, cooint_t y);
        void press(uint8_t b = MOUSE_LEFT);
        void release(uint8_t b = MOUSE_LEFT);
        bool isPressed(uint8_t b = MOUSE_LEFT);
    private:

        HWMouse m_mouse;
        std::pair<double, double> errs;

        void windMouse(
            cooint_t x, cooint_t y,
            double final_accuracy,
            int8_t G = 9, int8_t W = 5, int8_t M = 20, int8_t D = 12
        );
        void do_move(cooint_t x, cooint_t y);
};

