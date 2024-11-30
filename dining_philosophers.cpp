#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <cstdlib>   // For srand(), rand()
#include <ctime>     // For time()
#include <thread>
#include <vector>
#include <stdexcept> // For exceptions

// Constants for simulation parameters
#define DURATION 0.5                // Maximum additional duration to add to minimum times
#define MAX_FOOD 30               // Total amount of food available
#define MIN_THINK 0.5               // Minimum time a philosopher spends thinking
#define MIN_EAT 0.25                 // Minimum time a philosopher spends eating
#define NUM_FORK 7                // Number of forks (same as number of philosophers)
#define NUM_PHILOSOPHER 7         // Number of philosophers

using namespace std;

// Global shared variables and synchronization primitives
int food = MAX_FOOD;             // Shared food counter
mutex food_mutex;                // Mutex to protect access to the food variable
mutex forks[NUM_FORK];           // Mutexes representing the forks

mutex start_mtx;                 // Mutex for starting condition
condition_variable start_cv;     // Condition variable to synchronize the start of dining
bool start = false;              // Flag indicating whether philosophers can start dining

mutex report_mutex;              // Mutex to protect reporting output
double contention_time = 0;      // Track experimental result

// Philosopher class representing each philosopher in the simulation
class Philosopher {
private:
    int id;                          // Philosopher's ID
    mutex* fork1;                    // Pointer to the first fork
    mutex* fork2;                    // Pointer to the second fork
    bool done_eating = false;        // Flag to indicate whether the philosopher is done eating

    vector<double> thinkingTimes;    // Record of thinking times
    vector<double> hungryTimes;      // Record of hungry times (time spent waiting for forks)
    vector<double> eatingTimes;      // Record of eating times

    // Function to print the philosopher's activity times
    void report(){
        // Lock the report mutex to ensure exclusive access to output
        lock_guard<mutex> lock(report_mutex);
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
            contention_time += time;
        }
        cout << endl << endl;
    }

    // Function to simulate eating
    void eat(){
        // Lock the food mutex to protect access to the shared food counter
        food_mutex.lock();

        if (food > 0) {
            food--;
            cout << "PHILOSOPHER " << id << " IS EATING. FOOD LEFT: " << food << endl;
            food_mutex.unlock();
        } else {
            // No food left, philosopher is done eating
            food_mutex.unlock();
            this->done_eating = true;
            return;
        }

        // Sleep to simulate eating time
        this->sleep("eating", MIN_EAT, MIN_EAT + DURATION);
    }

    // Function to simulate sleeping (thinking or eating)
    void sleep(const string& verb, double min_time, double max_time){
        // Validate arguments
        if (min_time < 0 || max_time < 0 || max_time < min_time) {
            cerr << "PHILOSOPHER " << id << ": INVALID MIN AND MAX TIME" << endl;
            throw invalid_argument("Invalid min and max time for sleep");
        }

        // Generate random sleep duration between min_time and max_time
        double offset = static_cast<double>(rand()) / RAND_MAX * (max_time - min_time);
        double sleep_time = min_time + offset;

        // Simulate the activity
        this_thread::sleep_for(chrono::duration<double>(sleep_time));

        // Record the time spent in the corresponding activity
        if (verb == "eating") {
            this->eatingTimes.push_back(sleep_time);
        }
        else if (verb == "thinking") {
            this->thinkingTimes.push_back(sleep_time);
        }
        else {
            // Handle unexpected verb
            cerr << "PHILOSOPHER " << id << ": Unknown activity: " << verb << endl;
            throw invalid_argument("Unknown activity in sleep function");
        }
    }

    // Function to acquire forks (mutexes)
    void acquire_forks(){
        // Track time spent waiting for forks
        auto start_time = chrono::steady_clock::now();

        // Lock the two forks (mutexes)
        try {
            fork1->lock();
            fork2->lock(); 
        } catch (const system_error& e) {
            cerr << "PHILOSOPHER " << id << ": Failed to acquire forks: " << e.what() << endl;
            throw;
        }

        // Calculate time spent waiting for forks
        auto end_time = chrono::steady_clock::now();
        chrono::duration<double> elapsed = end_time - start_time;
        this->hungryTimes.push_back(elapsed.count());
    }

    // Function to release forks (mutexes)
    void release_forks(){
        // Unlock the forks
        try {
            fork1->unlock();
            fork2->unlock();
        } catch (const system_error& e) {
            cerr << "PHILOSOPHER " << id << ": Failed to release forks: " << e.what() << endl;
            throw;
        }
    }

public:
    // Operator() to make the Philosopher class callable (thread function)
    void operator()(int id, int fork1_index, int fork2_index){
        this->id = id;

        // Ensure fork indices are within bounds
        if (fork1_index < 0 || fork1_index >= NUM_FORK || fork2_index < 0 || fork2_index >= NUM_FORK) {
            cerr << "PHILOSOPHER " << id << ": Invalid fork indices" << endl;
            throw out_of_range("Fork index out of range");
        }

        this->fork1 = &forks[fork1_index];
        this->fork2 = &forks[fork2_index];

        // Wait until all philosophers are ready to start
        {
            unique_lock<mutex> lock(start_mtx);
            start_cv.wait(lock, []{ return start; });
        }

        // Start dining
        while (!done_eating) {
            try {
                this->sleep("thinking", MIN_THINK, MIN_THINK + DURATION);
                this->acquire_forks();
                this->eat();
                this->release_forks();
            } catch (const exception& e) {
                cerr << "PHILOSOPHER " << id << ": Exception occurred: " << e.what() << endl;
                // Ensure forks are released if an exception occurs
                try {
                    this->release_forks();
                } catch (...) {
                    // Ignore exceptions from release_forks
                }
                break;
            }
        }

        // Report activity times
        this->report();
    }
};

int main() {
    // Seed random number generator
    srand(static_cast<unsigned int>(time(NULL)));

    // Initialize philosophers
    vector<thread> philosophers;

    // Asymmetric solution for deadlock prevention
    try {
        for(int id = 0; id < NUM_PHILOSOPHER; id++){
            // Determine the fork indices based on philosopher's ID to prevent deadlock
            int fork1_index, fork2_index;
            if (id % 2) {
                fork1_index = id % NUM_FORK;
                fork2_index = (id + 1) % NUM_FORK;
            }
            else {
                fork1_index = (id + 1) % NUM_FORK;
                fork2_index = id % NUM_FORK;
            }

            // Create and launch philosopher threads
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
    {
        lock_guard<mutex> lock(start_mtx);
        start = true;
    }
    start_cv.notify_all();

    // Join all the threads
    for (thread &philosopher : philosophers){
        try {
            if (philosopher.joinable()) {
                philosopher.join();
            }
        } catch (const exception& e) {
            cerr << "Exception occurred while joining threads: " << e.what() << endl;
            // Continue joining other threads
        }
    }

    // Make sure food is modified correctly
    cout << "There is " << food << " food left." << endl;
    // See how much time was spent waiting
    cout << "Philosophers waited a total of " << contention_time << "s." << endl;

    return EXIT_SUCCESS;
}
