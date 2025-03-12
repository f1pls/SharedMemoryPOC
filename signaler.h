#include <iostream>
#include <fcntl.h>     // O_CREAT, O_RDWR
#include <sys/mman.h>  // shm_open, mmap
#include <unistd.h>    // ftruncate, close
#include <semaphore.h> // sem_t
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#ifndef SIGNALER_H
#define SIGNALER_H
struct SharedData{
    pthread_cond_t writer_cv;
    pthread_cond_t reader_cv;
    pthread_mutex_t lock;
    pthread_mutexattr_t attrmutex;
    long fileSize;
    bool writewait;
    bool readwait;
    volatile int wlock;
    volatile int num_of_reads;
    char value[];
};
void signal_next(SharedData* mem)
{
    if (mem->wlock > 0)
    {
        pthread_cond_broadcast(&mem->writer_cv);
    }
    else
    {
        pthread_cond_broadcast(&mem->reader_cv);
    }
}
void pthread_lock(SharedData* mem, bool readOrWrite){
    if (mem->readwait == mem->writewait == true){
        if (readOrWrite == 0){
        signal_next(mem);
        pthread_cond_wait(&mem->reader_cv, &mem->lock);
        }
    }
    else{
       if (readOrWrite == 0){
        pthread_cond_wait(&mem->reader_cv, &mem->lock);
        // std::cout<< "A"<<std::endl;
        }
        else{
            pthread_cond_wait(&mem->writer_cv, &mem->lock);
        }
    }
}
#endif