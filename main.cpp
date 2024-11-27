#include <chrono>
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
mutex forks[NUM_FORK];

class Philosopher {
private:
    int id;
    mutex* fork1;
    mutex* fork2;
    bool done_eating = false;

    void sleep(string verb, int min_time, int max_time){
        // check for appropriate min, max
        if (min_time < 0 || max_time < 0 || max_time < min_time) {
            cout << "INVALID MIN AND MAX TIME" << endl;
            return;
        }

        double offset = (static_cast<double>(rand()) / RAND_MAX) * (max_time - min_time);
        double sleep_time = min_time + offset;
        this_thread::sleep_for(chrono::duration<float>(sleep_time));
        cout << "Philosopher " << this->id <<  " finished " << verb << " for " << sleep_time << "s." << endl;
    }

    void acquire_forks(){
        fork1->lock();
        fork2->lock(); 
    }

    void eat(){
        if (food > 0) {
            food--;
            cout << "FOOD LEFT: " << food << endl;
            this->sleep("eating", MIN_EAT, MIN_EAT + DURATION);
        }
        else {
            this->done_eating = true;
        }
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

        while (!done_eating) {
            // step 1
            this->sleep("thinking", MIN_THINK, MIN_THINK + DURATION);
            // step 2
            this->acquire_forks();
            // step 3
            this->eat();
            // step 4
            this->release_forks();
        }
    }
};

int main() {
    srand(time(NULL));

    vector<thread> philosophers;

    // initialize philosophers
    for(int id = 0; id < NUM_PHILOSOPHER; id++){
        // avoid deadlocks using asymmetry
        if (id % 2) {
            // if odd, take from left then right
            philosophers.emplace_back(Philosopher(), id, id % NUM_FORK, (id + 1) % NUM_FORK);
        }
        else {
            // if even, take from right then left
            philosophers.emplace_back(Philosopher(), id, (id + 1) % NUM_FORK, id % NUM_FORK);
        }
    }

    cout << "Philosophers initialized." << endl;

    // terminate philosopher threads
    for (thread &philosopher: philosophers){
        if (philosopher.joinable()) {
            philosopher.join();
        }
    }

    return 0;
}
