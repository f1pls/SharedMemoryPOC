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
// #define OKTOWRITE "/condwrite"
// #define MESSAGE "/msg"
// const char* SEM_NAME = "/sem_mutex_example";
int main()
{
    int des_cond, des_msg, shm_fd;
    // int mode = S_IRWXU | S_IRWXG;
    shm_fd = shm_open("/shm_mutex_example", O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("failure on shm_open on des_mutex");
        exit(1);
    }
    size_t fileSize = 58598428;
    SharedData *mutex =(SharedData*) mmap(NULL ,fileSize,PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mutex == MAP_FAILED ) {
        perror("Error on mmap on mutex\n");
        exit(1);
    }
    long SIZE= mutex->fileSize;
 char* data = new char[fileSize];
    std::ofstream output("output.NEF", std::ios::binary);
    
    std::chrono::time_point<std::chrono::system_clock> starttime = std::chrono::system_clock::now();
    std::chrono::seconds fps{0};
    auto timelimit =std::chrono::seconds{10};
    long counter;
    while(true){
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        if (now  - starttime >= timelimit) {
            break;
        }
        counter++;
        
        while (mutex->wlock > 0){
            std::cout<<"WLOCK: " << mutex->wlock<< "NUM_OF_READS: "<< mutex->num_of_reads << std::endl;
            std::cout<< "WAIT CONDITION"<< std::endl;
            pthread_cond_wait(&mutex->reader_cv, &mutex->lock);
        }

        pthread_mutex_unlock(&mutex->lock);
        mutex->num_of_reads++;
        auto start = std::chrono::high_resolution_clock::now();
        data = mutex->value;
        auto end = std::chrono::high_resolution_clock::now();

        mutex->num_of_reads--;
        signal_next(mutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(14));
        double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
        if (starttime - now + fps <= std::chrono::seconds{0}){
            fps = fps + std::chrono::seconds{1};
            std::cout << "FPS: " << counter/std::chrono::seconds(1).count()<<std::endl;
            std::cout << "[Reader] Read value: " << mutex->value << std::endl;
            std::cout << "[Reader] Read size: " << mutex->fileSize << std::endl;    
            std::cout << "PNG file read from shared memory and saved as output.png\n";

            counter = 0;
            std::cout << "Read Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024 * 1024) << " MB/s" << std::endl;
            std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        }
    }
    output.write(data, mutex->fileSize);
    output.close();
    
    // Cleanup
    munmap(mutex,SIZE);
    close(shm_fd);

    return 0;
}


