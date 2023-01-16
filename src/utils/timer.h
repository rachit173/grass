#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <bits/stdc++.h>

class Timer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
public:
    inline long start() {
        begin = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(begin.time_since_epoch()).count();
    }

    inline void stop() {
        end = std::chrono::high_resolution_clock::now();
    }

    inline double now() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    double get_time_in_microseconds() {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    }

    double get_time_in_nanoseconds() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    }
};

#endif