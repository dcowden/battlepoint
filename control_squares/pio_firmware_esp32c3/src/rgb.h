#pragma once

#include <Arduino.h>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Rgb() : r(0), g(0), b(0) {}
    Rgb(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};
