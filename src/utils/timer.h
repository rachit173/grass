#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <bits/stdc++.h>

class AbstractTimer {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual double get_time_in_nanoseconds() = 0;
};

class Timer: public AbstractTimer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
public:
    inline void start() {
        begin = std::chrono::high_resolution_clock::now();
    }

    inline void stop() {
        end = std::chrono::high_resolution_clock::now();
    }

    inline double now() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    double get_time_in_nanoseconds() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    }
};

class Timer2: public AbstractTimer {
    private:
    struct timespec begin, end;
public:
    inline void start() {
        clock_gettime(CLOCK_MONOTONIC, &begin);
    }

    inline void stop() {
        clock_gettime(CLOCK_MONOTONIC, &end);
    }

    double get_time_in_nanoseconds() {
        double time_taken;
        time_taken = (end.tv_sec - begin.tv_sec) * 1e9;
        time_taken += (end.tv_nsec - begin.tv_nsec);
        return time_taken;
    }
};

#endif