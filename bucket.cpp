#include "bucket.hpp"

Bucket :: CheckResult Bucket :: check_and_consume_limit() {
    std::lock_guard<std::mutex> lock(mtx);
    refill();
    if (remaining >= 1) {
        remaining -= 1;
        return {true, capacity, remaining, 0};
    } else {
        auto now = std::chrono::steady_clock::now();
        auto time_since_last_refill = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_time).count();
        long long retry_after_ms = static_cast<long long>(std::ceil((1.0 / refill_rate) * 1000)) - time_since_last_refill;
        return {false, capacity, remaining, retry_after_ms > 0 ? retry_after_ms : 0};
    }
}

Bucket :: CheckResult Bucket :: get_limit() {
    std::lock_guard<std::mutex> lock(mtx);
    refill();
    return {remaining > 0, capacity, remaining, 0};
}

void Bucket :: configure(double new_capacity, double new_refill_rate) {
    std::lock_guard<std::mutex> lock(mtx);
    capacity = new_capacity;
    refill_rate = new_refill_rate;
    remaining = new_capacity;
    last_refill_time = std::chrono::steady_clock::now();
}