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
struct SharedData {
    pthread_cond_t reader_cv;
    pthread_cond_t writer_cv;
    pthread_mutex_t lock;
    size_t filesize;
    volatile int wlock;
    volatile int num_of_reads;
    char value[];
};
void signal_next(SharedData* shared)
{
    if (shared->wlock > 0)
    {
        // If any writes are waiting, wake one up
        pthread_cond_signal(&shared->writer_cv);
    }
    else
    {
        // If there are no writes pending, wake up all the
        // readers (there may not be any but that's fine)
        pthread_cond_broadcast(&shared->reader_cv);
    }
}
#endif