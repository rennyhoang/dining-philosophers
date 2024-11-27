#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdlib.h>
#include <thread>
#include <time.h>
#include <vector>

#define DURATION 2
#define MAX_FOOD 30
#define MIN_THINK 2
#define MIN_EAT 1
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

    vector<double> thinkingTimes;
    vector<double> waitingTimes;
    vector<double> eatingTimes;

    void report(){
        report_mutex.lock();
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

        cout << "PHILOSOPHER " << id << " WAITING TIMES: ";
        for (const double& time : waitingTimes) {
            cout << time << "s ";
        }
        cout << endl << endl;
        report_mutex.unlock();
    }

    void eat(){
        food_mutex.lock();

        if (food > 0) {
            food--;
            cout << "FOOD LEFT: " << food << endl;
            food_mutex.unlock();
        } else {
            food_mutex.unlock();
            this->done_eating = true;
            return;
        }

        this->sleep("eating", MIN_EAT, MIN_EAT + DURATION);
    }


    void sleep(string verb, int min_time, int max_time){
        // argument checking
        if (min_time < 0 || max_time < 0 || max_time < min_time) {
            cout << "INVALID MIN AND MAX TIME" << endl;
            return;
        }

        double offset = static_cast<double>(rand()) / RAND_MAX * (max_time - min_time);
        double sleep_time = min_time + offset;
        this_thread::sleep_for(chrono::duration<float>(sleep_time));

        if (!verb.compare("eating")) {
            this->eatingTimes.push_back(sleep_time);
        }
        if (!verb.compare("thinking")) {
            this->thinkingTimes.push_back(sleep_time);
        }
    }

    void acquire_forks(){
        auto start = chrono::steady_clock::now();
        fork1->lock();
        fork2->lock(); 
        auto end = chrono::steady_clock::now();
        chrono::duration<double> elapsed = end - start;
        this->waitingTimes.push_back(elapsed.count());
    }

    void release_forks(){
        fork1->unlock();
        fork2->unlock();
    }

public:
    void operator()(int id, int fork1, int fork2){
        this->id = id;
        this->fork1 = &forks[fork1];
        this->fork2 = &forks[fork2];

        // wait for thread initialization
        {
            unique_lock<mutex> lock(start_mtx);
            start_cv.wait(lock, []{ return start; });
        }

        // start dining
        while (!done_eating) {
            this->sleep("thinking", MIN_THINK, MIN_THINK + DURATION);
            this->acquire_forks();
            this->eat();
            this->release_forks();
        }

        this->report();
    }
};

int main() {
    // seed random
    srand(time(NULL));

    // initialize philosophers
    vector<thread> philosophers;

    // asymmetric solution for deadlock prevention
    for(int id = 0; id < NUM_PHILOSOPHER; id++){
        if (id % 2) {
            philosophers.emplace_back(Philosopher(), id, id % NUM_FORK, (id + 1) % NUM_FORK);
        }
        else {
            philosophers.emplace_back(Philosopher(), id, (id + 1) % NUM_FORK, id % NUM_FORK);
        }
    }

    cout << "Philosophers initialized." << endl;

    // make sure all philosophers start at same time
    cout << "Philosophers start dining in 3..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "2..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "1..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "EAT!!!" << endl;

    {
        lock_guard<mutex> lock(start_mtx);
        start = true;
    }
    start_cv.notify_all();

    // join all the threads
    for (thread &philosopher: philosophers){
        if (philosopher.joinable()) {
            philosopher.join();
        }
    }

    // make sure food modified correctly
    cout << "There is " << food << " food left." << endl;

    return 0;
}
