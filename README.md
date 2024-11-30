# Dining Philosophers Problem in C++

## Overview

This project is a C++ implementation of the classic Dining Philosophers Problem. It demonstrates synchronization and concurrency control using threads, mutexes, and condition variables. The program simulates philosophers who alternate between thinking and eating, ensuring that there is no deadlock or starvation.

## Solution Approach

To prevent deadlock, an asymmetric solution is implemented:

- Odd-numbered philosophers pick up the left fork first, then the right fork.
- Even-numbered philosophers pick up the right fork first, then the left fork.

This approach ensures that not all philosophers are competing for the same fork order, preventing a circular wait condition, which is one of the necessary conditions for a deadlock.

## Building the Project

1. Clone the repository

```bash
git clone https://github.com/yourusername/dining-philosophers-cpp.git
cd dining-philosophers-cpp
```

2. Compile the program

```bash
g++ -std=c++11 -pthread dining_philosophers.cpp -o dining_philosophers
```

## Running the program

After building the project, run the executable:

```bash
./dining_philosophers
```

## Program Output

The output includes:

- Initialization messages.
- Countdown before dining starts.
- Messages indicating when a philosopher is eating, thinking, or waiting.
- A report of each philosopher's eating times, thinking times, and hungry times.
- The amount of food left at the end.
