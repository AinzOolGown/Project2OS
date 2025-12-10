#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

class ProcessThread {
public:
    int pid, arrival, burst, priority;

    ProcessThread(int p, int a, int b, int pr)
        : pid(p), arrival(a), burst(b), priority(pr) {}

    void operator()() {
        cout << "[Process " << pid << "] Started. Burst: "
             << burst << " seconds" << endl;
        this_thread::sleep_for(chrono::seconds(burst));
        cout << "[Process " << pid << "] Finished." << endl;
    }
};

queue<int> bufferQ;
const int BUFFER_SIZE = 5;

mutex mtx;
condition_variable notFull, notEmpty;

bool productionDone = false;
int totalItems = 5;

void producerFunction(int id) {
    for (int i = 1; i <= totalItems; i++) {
        unique_lock<mutex> lock(mtx);
        cout << "[Producer " << id << "] Attempting to produce " << i << "..." << endl;

        notFull.wait(lock, [] { return bufferQ.size() < BUFFER_SIZE; });

        bufferQ.push(i);
        cout << "[Producer " << id << "] Produced item " << i
             << ". Buffer size: " << bufferQ.size() << endl;

        notEmpty.notify_one();
        lock.unlock();

        this_thread::sleep_for(chrono::milliseconds(300));
    }

    unique_lock<mutex> lock(mtx);
    productionDone = true;
    notEmpty.notify_all();
    cout << "[Producer " << id << "] Finished producing." << endl;
}

void consumerFunction(int id) {
    while (true) {
        unique_lock<mutex> lock(mtx);
        cout << "[Consumer " << id << "] Attempting to consume..." << endl;

        notEmpty.wait(lock, [] {
            return !bufferQ.empty() || productionDone;
        });

        if (bufferQ.empty() && productionDone) {
            cout << "[Consumer " << id << "] No more items. Exiting." << endl;
            return;
        }

        int item = bufferQ.front();
        bufferQ.pop();
        cout << "[Consumer " << id << "] Consumed item " << item
             << ". Buffer size: " << bufferQ.size() << endl;

        notFull.notify_one();
        lock.unlock();

        this_thread::sleep_for(chrono::milliseconds(250));
    }
}

int main() {
    cout << "===== PART 1: Reading processes.txt and launching threads =====" << endl;

    ifstream infile("processes.txt");
    if (!infile.is_open()) {
        cout << "Error: Could not open processes.txt" << endl;
        return 1;
    }

    vector<thread> processThreads;
    int pid, arrival, burst, priority;

    while (infile >> pid >> arrival >> burst >> priority) {
        processThreads.emplace_back(ProcessThread(pid, arrival, burst, priority));
    }

    for (auto& th : processThreads) {
        if (th.joinable())
            th.join();
    }

    cout << endl << "===== PART 2: Producer–Consumer Synchronization =====" << endl;

    thread producer(producerFunction, 1);
    thread consumer1(consumerFunction, 1);
    thread consumer2(consumerFunction, 2);

    producer.join();
    consumer1.join();
    consumer2.join();

    cout << "===== PROGRAM COMPLETE =====" << endl;
    return 0;
}
