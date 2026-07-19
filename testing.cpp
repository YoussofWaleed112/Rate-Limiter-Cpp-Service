#include <iostream>
#include <thread>
#include <chrono>
#include "rate_limiter_service.hpp"

using namespace std;

int main()
{
    Rate_Limiter limiter(5.0, 1.0);

    string key;
    int choice;

    do
    {
        cout << "\n========== Rate Limiter Manual Testing ==========\n";
        cout << "1. Generate API Key\n";
        cout << "2. Check API Key\n";
        cout << "3. Consume Token\n";
        cout << "4. Check Bucket Status\n";
        cout << "5. Configure Bucket\n";
        cout << "6. Wait 3 Seconds (Refill Test)\n";
        cout << "0. Exit\n";

        cout << "\nEnter Choice: ";
        cin >> choice;

        switch(choice)
        {
        case 1:
            key = limiter.generate_key();
            cout << "\nGenerated Key: " << key << endl;
            break;

        case 2:
        {
            string inputKey;
            cout << "Enter API Key: ";
            cin >> inputKey;

            if(limiter.check_key(inputKey))
                cout << "Valid Key\n";
            else
                cout << "Invalid Key\n";

            break;
        }

        case 3:
        {
            string inputKey;
            cout << "Enter API Key: ";
            cin >> inputKey;

            auto result = limiter.get_bucket_and_consume(inputKey);

            cout << "\nAllowed : " << result.allowed << endl;
            cout << "Limit : " << result.limit << endl;
            cout << "Remaining : " << result.remaining << endl;
            cout << "Retry After(ms): " << result.retry_after_ms << endl;

            break;
        }

        case 4:
        {
            string inputKey;
            cout << "Enter API Key: ";
            cin >> inputKey;

            auto result = limiter.check_bucket(inputKey);

            cout << "\nAllowed : " << result.allowed << endl;
            cout << "Limit : " << result.limit << endl;
            cout << "Remaining : " << result.remaining << endl;
            cout << "Retry After(ms): " << result.retry_after_ms << endl;

            break;
        }

        case 5:
        {
            string inputKey;
            double capacity, refill;

            cout << "Enter API Key: ";
            cin >> inputKey;

            cout << "Enter New Capacity: ";
            cin >> capacity;

            cout << "Enter New Refill Rate: ";
            cin >> refill;

            if(limiter.set_configuration(inputKey, capacity, refill))
                cout << "Configuration Updated Successfully.\n";
            else
                cout << "Invalid API Key.\n";

            break;
        }

        case 6:

            cout << "Waiting 3 Seconds...\n";
            this_thread::sleep_for(chrono::seconds(3));
            cout << "Done.\n";

            break;

        case 0:
            cout << "Program Ended.\n";
            break;

        default:
            cout << "Invalid Choice.\n";
        }

    } while(choice != 0);

    return 0;
}
