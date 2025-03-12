#include <iostream>
#include <fcntl.h>     // O_CREAT, O_RDWR
#include <sys/mman.h>  // shm_open, mmap
#include <unistd.h>    // ftruncate, close
#include <semaphore.h> // sem_t
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#include "signaler.h"
#define OKTOWRITE "/condwrite"
#define MESSAGE "/msg"
const char* SEM_NAME = "/sem_mutex_example";
int main()
{
    int des_cond, des_msg, shm_fd;
    int mode = S_IRWXU | S_IRWXG;
    shm_fd = shm_open("/shm_mutex_example", O_CREAT | O_RDWR, 0777);
    std::ifstream file("./bins/_Z723130.NEF", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    size_t fileSize = file.tellg();
    std:: cout<<fileSize <<" B"<<std::endl;
    file.seekg(0, std::ios::beg);
    if (shm_fd < 0) {
        perror("failure on shm_open on des_mutex");
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(SharedData)+fileSize) == -1) {
        perror("Error on ftruncate to sizeof pthread_cond_t\n");
        exit(-1);
    }

    void *lock =mmap(NULL, sizeof(SharedData)+fileSize,
            PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (lock == MAP_FAILED ) {
        perror("Error on mmap on mutex\n");
        exit(1);
    }
    SharedData *mutex = new (lock) SharedData;
    pthread_mutexattr_init(&mutex->attrmutex);
    pthread_mutexattr_setpshared(&mutex->attrmutex, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex->lock, &mutex->attrmutex);
    pthread_condattr_t attrcond;
    pthread_condattr_init(&attrcond);
    pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&mutex->writer_cv, &attrcond);
    pthread_cond_init(&mutex->reader_cv, &attrcond);
    mutex->fileSize=fileSize;
    std::chrono::seconds fps{0};
     std::chrono::time_point<std::chrono::system_clock> starttime = std::chrono::system_clock::now();
    auto timelimit =std::chrono::minutes{1};
    long counter;
    while(true){
        mutex->wlock++;
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        // if (now  - starttime >= timelimit) {
        // break;
        // }
        auto start = std::chrono::high_resolution_clock::now();
            if (mutex->wlock > 1 || mutex->num_of_reads > 0){
                if (mutex->writewait == mutex->readwait == true){
                    signal_next(mutex);
                }
                mutex->writewait=true;
                pthread_cond_wait(&mutex->writer_cv, &mutex->lock);
                mutex->writewait=false;
            }
        pthread_mutex_lock(&mutex->lock);
     
        // Writing data
                                    // Measure write time
        pthread_mutex_unlock(&mutex->lock);
        file.read(mutex->value, fileSize);
        pthread_mutex_lock(&mutex->lock);
        mutex->wlock--;
        signal_next(mutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(14));
        pthread_mutex_unlock(&mutex->lock);
        counter++;
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end - start).count();
        //     // Unlock
        if (starttime - now + fps <= std::chrono::seconds{0}){
        fps = fps + std::chrono::seconds{1};
        std::cout << "FPS: " << counter/fps.count()<<std::endl;
        std::cout << "[Writer] Wrote value: " << "SIZE= "<<fileSize<< std::endl;
        std::cout << "Write Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024* 1024) << " MB/s" << std::endl;
        std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        }
    // Cleanup
    }
    munmap(mutex, sizeof(mutex));
    close(shm_fd);
    return 0;
}
