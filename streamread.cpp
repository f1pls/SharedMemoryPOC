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
int main()
{
    int des_cond, des_msg, shm_fd;
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
    auto timelimit =std::chrono::seconds{60};
    long counter;
    while(true){
        auto start = std::chrono::high_resolution_clock::now();
        pthread_mutex_lock(&mutex->lock);
        while (mutex->wlock > 0){
            mutex->readwait=true;
            pthread_lock(mutex, 0);
            mutex->readwait=false;
        }
        mutex->num_of_reads++;
       

        pthread_mutex_unlock(&mutex->lock);
        data = mutex->value;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); //debug timeout to see performance with slow reader
        output.write(data, mutex->fileSize);
        signal_next(mutex);
        mutex->num_of_reads--;
        counter++;
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end - start).count();
        std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        if (now  - starttime >= timelimit) {
            break;
        }
        
        if (starttime - now + fps <= std::chrono::seconds{0}){
            fps = fps + std::chrono::seconds{1};                                 
            std::cout << "FPS: " << counter<<std::endl; 
            counter = 0; 
            std::cout << "[Reader] Read value: " << mutex->value << std::endl;           
            std::cout << "[Reader] Read size: " << mutex->fileSize << std::endl;    
            std::cout << "PNG file read from shared memory and saved as output.png\n";
            std::cout << "Read Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024 * 1024) << " MB/s" << std::endl;
            std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl; 
        }
        output.close();
    }
    
    // Cleanup
    munmap(mutex,SIZE);
    close(shm_fd);

    return 0;
}


