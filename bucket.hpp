#pragma once

#include <mutex>
#include <chrono>
#include <algorithm>
#include <cmath>

class Bucket {

public:

        struct CheckResult {
        bool allowed;
        double limit;
        double remaining;
        long long retry_after_ms; // 0 when allowed
    };

    Bucket(double capacity, double refill_rate)
        : capacity(capacity), remaining(capacity), refill_rate(refill_rate),
          last_refill_time(std::chrono::steady_clock::now()) {}

    CheckResult check_and_consume_limit();
    CheckResult get_limit();
    void configure(double new_capacity, double new_refill_rate);

private :
    void refill(){
        auto now = std::chrono::steady_clock::now();
        auto time_since_last_refill = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_time).count();
        double tokens_to_add = (time_since_last_refill / 1000.0) * refill_rate;
        remaining = std::min(capacity, remaining + tokens_to_add);
        last_refill_time = now;
    }
    double capacity;
    double remaining;
    double refill_rate; // tokens per second
    std::chrono::steady_clock::time_point last_refill_time;
    std::mutex mtx;


};