#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#define DURATION 0.5
#define MAX_FOOD 30
#define MIN_THINK 0.5
#define MIN_EAT 0.5
#define NUM_FORK 7 
#define NUM_PHILOSOPHER 7

using namespace std;

int food = MAX_FOOD; 
mutex food_mutex;   
mutex forks[NUM_FORK];

mutex start_mtx;     
condition_variable start_cv;
bool start = false;        

mutex report_mutex;       

class Philosopher {
private:
    int id;
    mutex* fork1;
    mutex* fork2;
    bool done_eating = false;

    vector<double> hungryTimes;
    vector<double> thinkingTimes;
    vector<double> eatingTimes;

    void report(){
        report_mutex.lock();
        cout << endl;
        cout << "PHILOSOPHER " << id << " EATING TIMES: ";
        for (auto &time : eatingTimes) {
            cout << time << "s ";
        }
        cout << endl;

        cout << "PHILOSOPHER " << id << " THINKING TIMES: ";
        for (const double& time : thinkingTimes) {
            cout << time << "s ";
        }
        cout << endl;

        cout << "PHILOSOPHER " << id << " HUNGRY TIMES: ";
        for (const double& time : hungryTimes) {
            cout << time << "s ";
        }
        report_mutex.unlock();

        double hungryTime = accumulate(hungryTimes.begin(), hungryTimes.end(), 0.0);
        double thinkingTime = accumulate(thinkingTimes.begin(), thinkingTimes.end(), 0.0);
        double eatingTime = accumulate(eatingTimes.begin(), eatingTimes.end(), 0.0);

        double overallTime = hungryTime + thinkingTime + eatingTime;

        double contention_level = hungryTime / overallTime * 100;

        ofstream reportFile("report-" + to_string(id) + ".txt", ios::app);
        reportFile << contention_level << endl;
        reportFile.close();
    }

    void eat(){
        food_mutex.lock();

        if (food > 0) {
            food--;
            cout << "PHILOSOPHER " << id << " IS EATING. FOOD LEFT: " << food << endl;
            food_mutex.unlock();
        } else {
            food_mutex.unlock();
            this->done_eating = true;
            return;
        }

        this->sleep("eating", MIN_EAT, MIN_EAT + DURATION);
    }

    void sleep(const string& verb, double min_time, double max_time){
        if (min_time < 0 || max_time < 0 || max_time < min_time) {
            cerr << "PHILOSOPHER " << id << ": INVALID MIN AND MAX TIME" << endl;
            throw invalid_argument("Invalid min and max time for sleep");
        }

        double offset = static_cast<double>(rand()) / RAND_MAX * (max_time - min_time);
        double sleep_time = min_time + offset;

        this_thread::sleep_for(chrono::duration<double>(sleep_time));

        if (verb == "eating") {
            this->eatingTimes.push_back(sleep_time);
        }
        else if (verb == "thinking") {
            this->thinkingTimes.push_back(sleep_time);
        }
        else {
            cerr << "PHILOSOPHER " << id << ": Unknown activity: " << verb << endl;
            throw invalid_argument("Unknown activity in sleep function");
        }
    }

    void acquire_forks(){
        auto start_time = chrono::steady_clock::now();

        try {
            fork1->lock();
            fork2->lock(); 
        } catch (const system_error& e) {
            cerr << "PHILOSOPHER " << id << ": Failed to acquire forks: " << e.what() << endl;
            throw;
        }

        auto end_time = chrono::steady_clock::now();
        chrono::duration<double> elapsed = end_time - start_time;
        this->hungryTimes.push_back(elapsed.count());
    }

    void release_forks(){
        try {
            fork1->unlock();
            fork2->unlock();
        } catch (const system_error& e) {
            cerr << "PHILOSOPHER " << id << ": Failed to release forks: " << e.what() << endl;
            throw;
        }
    }

public:
    void operator()(int id, int fork1_index, int fork2_index){
        this->id = id;

        if (fork1_index < 0 || fork1_index >= NUM_FORK || fork2_index < 0 || fork2_index >= NUM_FORK) {
            cerr << "PHILOSOPHER " << id << ": Invalid fork indices" << endl;
            throw out_of_range("Fork index out of range");
        }

        this->fork1 = &forks[fork1_index];
        this->fork2 = &forks[fork2_index];

        {
            unique_lock<mutex> lock(start_mtx);
            start_cv.wait(lock, []{ return start; });
        }

        while (!done_eating) {
            try {
                this->sleep("thinking", MIN_THINK, MIN_THINK + DURATION);
                this->acquire_forks();
                this->eat();
                this->release_forks();
            } catch (const exception& e) {
                cerr << "PHILOSOPHER " << id << ": Exception occurred: " << e.what() << endl;
                // Make sure forks are released if an exception occurs
                this->release_forks();
                break;
            }
        }

        this->report();
    }
};

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    vector<thread> philosophers;

    try {
        for(int id = 0; id < NUM_PHILOSOPHER; id++){
            int fork1_index, fork2_index;
            if (id % 2) {
                fork1_index = id % NUM_FORK;
                fork2_index = (id + 1) % NUM_FORK;
            }
            else {
                fork1_index = (id + 1) % NUM_FORK;
                fork2_index = id % NUM_FORK;
            }

            philosophers.emplace_back(Philosopher(), id, fork1_index, fork2_index);
        }
    } catch (const exception& e) {
        cerr << "Failed to create philosopher threads: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    cout << "Philosophers initialized." << endl;

    // Make sure all philosophers start at the same time
    cout << "Philosophers start dining in 3..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "2..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "1..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "EAT!!!" << endl;

    // Notify all philosophers to start dining
    start_mtx.lock();
    start = true;
    start_mtx.unlock();
    start_cv.notify_all();

    for (thread &philosopher : philosophers){
        try {
            if (philosopher.joinable()) {
                philosopher.join();
            }
        } catch (const exception& e) {
            cerr << "Exception occurred while joining threads: " << e.what() << endl;
        }
    }

    cout << endl << endl;
    cout << "There is " << food << " food left." << endl;

    return EXIT_SUCCESS;
}
