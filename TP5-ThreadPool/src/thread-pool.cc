/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), done(false) {
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].available = true;
        availableWorkers.signal(); // Señalizar que hay workers disponibles
    }
    //Creo el dispatcher y lo pongo en ejecución
    dt = thread([this]{dispatcher();});
    //Creo los hilos workers y los pongo en ejecución
    for (size_t workerId = 0; workerId < numThreads; workerId++) {
        wts[workerId].ts = thread([this, workerId] { worker(workerId); });
    }
}

void ThreadPool::dispatcher(){
    while(!done){
        newTaskAvailable.wait();
        if(done) break;

        function<void(void)> task;
        {
            lock_guard<mutex> lock(queueLock); //bloquea la cola
            if(taskQueue.empty()){
                continue;
            }
            task = taskQueue.front();
            taskQueue.pop();
        }

        availableWorkers.wait();
        if(done) break;

        {
            lock_guard<mutex> lock(workerLock); //ningun otro hilo puede modificar el estado de los workers
            for(size_t i = 0; i < wts.size(); i++){
                if(wts[i].available){
                    wts[i].available = false; //marco worker como ocupado
                    wts[i].thunk = task; //asigno tarea al worker
                    wts[i].workReady.signal(); //lo despierto

                    break;
                }
            }
        }
    }
}

void ThreadPool::worker(int id){
    while (!done) {
        wts[id].workReady.wait();
        if(done) break;

        if(wts[id].thunk){
            wts[id].thunk(); //ejecuto tarea
            wts[id].thunk = nullptr; //limpio la tarea
        }

        {
            //el worker vuelve a estar disponible
            lock_guard<mutex> lock(workerLock);
            wts[id].available = true;
        }

        availableWorkers.signal(); //notificar worker disponible

    }

}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (destroyed) throw std::runtime_error("ThreadPool: schedule() called after destruction");
    if (!thunk) throw std::invalid_argument("ThreadPool: schedule() called with nullptr");
    {
        lock_guard<mutex> lock(queueLock); //bloquea cola
        taskQueue.push(thunk); //añade tarea a la cola
    }

    newTaskAvailable.signal();
}

void ThreadPool::wait() {
    if (destroyed) throw std::runtime_error("ThreadPool: wait() called after destruction");
    while(true){
        {
            lock_guard<mutex> lock1(queueLock);
            lock_guard<mutex> lock2(workerLock);

            bool allWorkersFree = true;
            for(const auto& w : wts){
                if(!w.available){
                    allWorkersFree = false;
                    break;
                }
            }
            if(taskQueue.empty() && allWorkersFree){
                break;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

// ThreadPool::~ThreadPool() {
//     // Marcar que debe terminar
//     done = true;

//     for (size_t i = 0; i < wts.size(); i++) {
//         wts[i].workReady.signal();
//     }

//     wait();
    
//     // Esperar a que terminen todos los hilos
//     if (dt.joinable()) dt.join();
    
//     for (auto& worker : wts) {
//         if (worker.ts.joinable()) worker.ts.join();
//     }
// }

ThreadPool::~ThreadPool() {
    wait();
    destroyed = true;
    done = true;
    newTaskAvailable.signal(); // Despertar dispatcher
    availableWorkers.signal(); // Despertar dispatcher si está esperando worker
    for (size_t i = 0; i < wts.size(); i++) {
        wts[i].workReady.signal(); // Despertar todos los workers
    }
    if (dt.joinable()) dt.join();
    for (auto& worker : wts) {
        if (worker.ts.joinable()) worker.ts.join();
    }
}
