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
git clone https://github.com/rennyhoang/dining-philosophers.git
cd dining-philosophers-cpp
```

2. Compile the program

```bash
g++ dining_philosophers.cpp
```

## Running the program

After building the project, run the executable:

```bash
./a.out
```

## Program Output

The output includes:

- Initialization message.
- Countdown before dining starts.
- Messages indicating when a philosopher is eating.
- A report of each philosopher's eating times, thinking times, and hungry times.
- The amount of food left at the end.

Each philosopher will also write their contention times to a text file called "report-<id>.txt" 
