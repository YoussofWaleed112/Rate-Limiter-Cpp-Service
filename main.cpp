#include "httplib.h"
#include "rate_limiter_service.hpp"
#include "json.hpp"
#include <string>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

int main() {
    httplib::Server svr;
    Rate_Limiter limiter(5.0, 1.0); // Default capacity and refill rate

    std :: cout << "Welcome to the Rate Limiter Service API" << std::endl;
    svr.Post("/Request-Key", [&](const httplib::Request& req, httplib::Response& res) {
        std :: string key = limiter.generate_key();

        std :: cout << "Key generated successfully : " + key << std::endl;
        json response = {
            {"message", "API key generated , hello there"},
            {"your_key", key},
            {"limit", 5.0},
            {"requests_per_second", 1.0}
        };

        res.status = 201; // Successful


        res.set_content(response.dump(), "application/json");
    });


    svr.Post("/check", [&](const httplib::Request& req, httplib::Response& res) {
        std :: cout << "Received request to check key" << std::endl;
        json incoming_request = json::parse(req.body);

        
        if (!incoming_request.contains("api_key") || incoming_request["api_key"].get<std::string>().empty() || !incoming_request["api_key"].is_string()) {
            res.status = 400; // Bad Request
            json response = {
                {"error", "Invalid API key format, please provide a valid key format."}
            };
            std :: cout << "Missing / Invalid format for key in request" << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }

        std :: string key = incoming_request["api_key"].get<std::string>();
        bool check = limiter.check_key(key);
        std :: cout << "Checking key: " + key << std::endl;
        if (!check){ 
            json response = {
                {"error", "Invalid API key, please provide a valid Authentication key."}
            };
            res.status = 401; // Unauthorized
            std :: cout << "Unauthorized / wrong access attempt with invalid key" << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }
        std :: cout << "Key is valid, checking bucket for key: " + key << std::endl;
        auto bucket= limiter.get_bucket_and_consume(key);

        json response = {
            {"Access", "bucket.allowed"},
            {"Limit", bucket.limit},
            {"Remaining", bucket.remaining},
            {"Retry_after_ms", bucket.retry_after_ms}
        };

         if (bucket.allowed) {
            std :: cout << "Access granted for key: " + key << std::endl;
            res.status = 200;
        } else {
            response["retry_after_ms"] = bucket.retry_after_ms;
            std :: cout << "Access denied for key: " + key + " , retry after : " + std::to_string(bucket.retry_after_ms) + " ms" << std::endl;
            res.status = 429; // Too Many Requests
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/status/key", [&](const httplib::Request& req, httplib::Response& res) {
        json response;
        std :: cout << "Received request to check status of key" << std::endl;
        std :: string key = req.get_param_value("api_key");
        auto bucket = limiter.check_bucket(key);
        

        if (!key.empty() && bucket.allowed) {
            json response= {
                {"limit", bucket.limit},
                {"remaining", bucket.remaining},
                {"retry_after_ms", bucket.retry_after_ms}
            };
            res.status = 200; // Successful
            res.set_content(response.dump(), "application/json");
        }
        
        else if(key.empty()){
            res.status = 400; // Bad Request
            json response = {
                {"error", "Missing API key in request, please provide a valid key."}
            };
            std :: cout << "Missing API key in request" << std::endl;
            res.set_content(response.dump(), "application/json");
        }

        else if(!bucket.allowed){
            json response= {
                {"limit", bucket.limit},
                {"remaining", bucket.remaining},
                {"retry_after_ms", bucket.retry_after_ms}
            };
            res.status = 429; // Too Many Requests
            std :: cout << "Access denied for key: " + key + " , retry after : " + std::to_string(bucket.retry_after_ms) + " ms" << std::endl;
            res.set_content(response.dump(), "application/json");
        }
        
        else{
            res.status = 401; // Unauthorized
            json response = {
                {"error", "Invalid API key, please provide a valid Authentication key."}
            };
            std :: cout << "Unauthorized / wrong access attempt with invalid key" << std::endl;
            res.set_content(response.dump(), "application/json");
        }
        
       
    });

    svr.Post("/configure", [&](const httplib::Request& req, httplib::Response& res) {
        json incoming_request = json::parse(req.body);

        if (!incoming_request.contains("api_key") || incoming_request["api_key"].get<std::string>().empty() || !incoming_request["api_key"].is_string()) {
            res.status = 400; // Bad Request
            json response = {
                {"error", "Invalid API key format, please provide a valid key format."}
            };
            std :: cout << "Missing / Invalid format for key in request" << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }

        else if (!incoming_request.contains("capacity") || !incoming_request["capacity"].is_number()) {
            res.status = 400; // Bad Request
            json response = {
                {"error", "Invalid capacity format, please provide a valid number for capacity."}
            };
            std :: cout << "Missing / Invalid format for capacity in request" << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }

        else if (!incoming_request.contains("refill_rate") || !incoming_request["refill_rate"].is_number()) {
            res.status = 400; // Bad Request
            json response = {
                {"error", "Invalid refill rate format, please provide a valid number for refill rate."}
            };
            std :: cout << "Missing / Invalid format for refill rate in request" << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }

        std :: string key = incoming_request["api_key"].get<std::string>();
        double capacity = incoming_request["capacity"].get<double>();
        double refill_rate = incoming_request["refill_rate"].get<double>();

        bool configure = limiter.set_configuration(key, capacity, refill_rate);

        if(!configure){
            json response = {
                {"error", "Failed to configure rate limit for the provided API key."}
            };
            res.status = 401; // Unauthorized
            std :: cout << "Error during configuration attempt with key: " << key << std::endl;
            res.set_content(response.dump(), "application/json");
            return;
        }
        
        json response = {
            {"message", "Rate limit configuration updated successfully for the provided API key."},
            {"new_limit", capacity},
            {"new_requests_per_second", refill_rate}
        };

        res.status = 200; // Successful
        std :: cout << "Rate limit configuration updated successfully for key: " << key << std::endl;
        res.set_content(response.dump(), "application/json");
        return;


    });   



    svr.listen("localhost", 8080);

    return 0;
}

