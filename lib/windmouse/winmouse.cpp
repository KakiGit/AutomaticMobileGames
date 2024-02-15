#include "windmouse.h"
#include "math.h"
#include "esp_random.h"
#include "USB.h"
#include <esp_log.h>

const static double sqrt3 = sqrt(3);
const static double sqrt5 = sqrt(5);

double z_rand() {
    return 2 * ((double)rand())/RAND_MAX - 1;
}

void USBSetup() {
  ESP_LOGI("", "Initializing USB");
  USB.VID(0x0817);
  USB.PID(0xA133);
  USB.productName("Magic 2");
  USB.manufacturerName("BSoft");
  USB.begin();
}

WindMouse::WindMouse() {
    ESP_LOGI("", "Starting Mouse");
    m_mouse = new HWMouse();
    USBSetup();
    m_mouse->begin();
    errs = std::pair<double, double>(0, 0);
    m_queue = RDB.subscribe("POSITION");
    srand(esp_random());
}

WindMouse::~WindMouse() {
    m_mouse->end();
    delete m_mouse;
}

void WindMouse::pullAndMove() {
    std::pair<std::string, std::string> location = m_queue->pop();
    syncParams();
    cooint_t x = std::stoi(location.first);
    cooint_t y = std::stoi(location.second);
    ESP_LOGI(""," x %d y %d", x, y);
    move(x, y);
}

void WindMouse::move(cooint_t x, cooint_t y) {
    double accuracy = final_accuracy * ((double)rand())/RAND_MAX;
    windMouse(
        round(x - errs.first),
        round(y - errs.second),
        accuracy
    );
}

void WindMouse::press(uint8_t b) {
    m_mouse->press(b);
}

bool WindMouse::isPressed(uint8_t b) {
    return m_mouse->isPressed(b);
}

void WindMouse::release(uint8_t b) {
    m_mouse->release(b);
}

void WindMouse::do_syncParam(const std::string& key, int8_t& param) {
    std::string value = RDB.read(key);
    if (!value.empty()) {
        param = std::stoi(value);
    } else {
        ESP_LOGI("", "Not able to sync %s", key);
    }
}

void WindMouse::do_syncParam(const std::string& key, int16_t& param) {
    std::string value = RDB.read(key);
    if (!value.empty()) {
        param = std::stoi(value);
    } else {
        ESP_LOGI("", "Not able to sync %s", key);
    }
}

void WindMouse::do_syncParam(const std::string& key, double& param) {
    std::string value = RDB.read(key);
    if (!value.empty()) {
        param = std::stod(value);
    } else {
        ESP_LOGI("", "Not able to sync %s", key);
    }
}

void WindMouse::syncParams() {
    std::string Gs = "G", Ws = "W", Ms = "M", Ds = "D";
    do_syncParam("G", G);
    do_syncParam("W", W);
    do_syncParam("M", M);
    do_syncParam("D", D);
    do_syncParam("final_accuracy", final_accuracy);
    do_syncParam("report_rate", report_rate);
}

void WindMouse::do_move(cooint_t x, cooint_t y) {
    m_mouse->move(x, y);
    delay(1000/report_rate);
}

void WindMouse::windMouse(cooint_t dest_x, cooint_t dest_y, double final_accuracy) {
    double M_0 = M;
    double M_0_0 = M_0;
    cooint_t current_x, current_y;
    double start_x, start_y;
    double W_x, W_y, v_x, v_y;
    cooint_t r_dest_x = dest_x + final_accuracy * z_rand();
    cooint_t r_dest_y = dest_y + final_accuracy * z_rand();
    double distribution = 0.5 * final_accuracy;
    double orig_dist = hypot(r_dest_x - start_x, r_dest_y - start_y);
    for(double dist = hypot(r_dest_x - start_x, r_dest_y - start_y); dist >= distribution;) {
        double W_mag = fmin(W, dist);

        if (dist >= D) {
            double x_rand = z_rand();
            double y_rand = z_rand();

            W_x = W_x/sqrt3 + x_rand * W_mag/sqrt5;
            W_y = W_y/sqrt3 + y_rand * W_mag/sqrt5;
            M_0 = M_0_0 * dist / orig_dist;
        } else {
            W_x /= sqrt3;
            W_y /= sqrt3;
            if ( M_0 < 3.0) {
                M_0 = ((double)rand())/RAND_MAX * 3.0 + 3.0;
            } else {
                M_0 /= sqrt5;
            }
        }

        v_x += W_x + ((double)G) * ( ((double)r_dest_x) - start_x ) / dist;
        v_y += W_y + ((double)G) * ( ((double)r_dest_y) - start_y ) / dist;

        double v_mag = hypot(v_x, v_y);

        if (v_mag > M_0) {
            double v_clip = M_0 / 2.0 + ((double)rand())/RAND_MAX * M_0 / 2.0;
            v_x = ( v_x / v_mag ) * v_clip;
            v_y = ( v_y / v_mag ) * v_clip;
        }

        start_x += v_x;
        start_y += v_y;
        cooint_t move_x = cooint_t(round(start_x));
        cooint_t move_y = cooint_t(round(start_y));

        if ( current_x != move_x || current_y != move_y ) {
            do_move(move_x - current_x, move_y - current_y);
            current_x = move_x;
            current_y = move_y;
        }
        dist = hypot(r_dest_x - start_x, r_dest_y - start_y);
    }
    errs.first = start_x - dest_x;
    errs.second = start_y - dest_y;
}