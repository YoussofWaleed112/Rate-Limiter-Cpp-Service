#include <unordered_map>
#include "bucket.hpp"
#include <string>
#include <mutex>
#include <random>

class Rate_Limiter{

    public: 

    Rate_Limiter(double capacity, double refill_rate) : default_capacity(capacity), default_refill_rate(refill_rate) {}
    std ::string generate_key();
    bool check_key(const std::string& key);
    bool set_configuration(const std::string& key, double new_capacity, double new_refill_rate);
    Bucket :: CheckResult get_bucket_and_consume(const std::string& key);
    Bucket :: CheckResult check_bucket(const std::string& key);


    private:

    std ::unordered_map<std::string, Bucket> buckets;
    std :: mutex mtx;
    double default_capacity = 5.0; // Default capacity for all buckets
    double default_refill_rate = 1.0; // Default refill rate for all buckets (tokens per second)
    std :: mt19937 rng{std::random_device{}()}; // Random number generator for key generation


};