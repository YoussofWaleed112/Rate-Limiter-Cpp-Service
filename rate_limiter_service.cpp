#include "rate_limiter_service.hpp"

std :: string Rate_Limiter :: generate_key() {
        std::lock_guard<std::mutex> lock(mtx);
        std::string key = "";
        const char letters[] = "abcdefghijklmnopqrstuvwxyz";
        const char digits[] = "0123456789";
        std :: uniform_int_distribution<int> letter_dist(0,25);
        std :: uniform_int_distribution<int> digit_dist(0,9);

       for(auto i{0}; i<3 ; i++){
        key+= letters[letter_dist(rng)];
       }

       for(auto i{0}; i<3 ; i++){
        key+= digits[digit_dist(rng)];
       }

        buckets.try_emplace(key, default_capacity, default_refill_rate);
        return key;
    }

bool Rate_Limiter :: check_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = buckets.find(key);
        if (it == buckets.end()) {
            return false; // Key not found
        }
        return true; // Key exists
    }

Bucket :: CheckResult Rate_Limiter :: get_bucket_and_consume(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = buckets.find(key);
        if (it == buckets.end()) {
            return {false, 0, 0, 0}; // Key not found
        }
        return it->second.check_and_consume_limit();
    }

Bucket :: CheckResult Rate_Limiter :: check_bucket(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = buckets.find(key);
        if (it == buckets.end()) {
            return {false, 0, 0, 0}; // Key not found
        }
        return it->second.get_limit();
    }

bool Rate_Limiter :: set_configuration(const std::string& key, double new_capacity, double new_refill_rate) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = buckets.find(key);
        if (it == buckets.end()) {
            return false; // Key not found
        }
        it->second.configure(new_capacity, new_refill_rate);
        return true;
    }
