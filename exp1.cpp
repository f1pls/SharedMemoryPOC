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

const char* SHM_NAME = "/shm_mutex_example";
const char* SEM_NAME = "/sem_mutex_example";
const int SIZE = 24*1024*1024;

int main() {
    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    std::ifstream file("Nikon-D4-Shotkit-2.NEF", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    // Set size of shared memory
    size_t fileSize = file.tellg();
    std:: cout<<fileSize/(1024*1024) <<" MB"<<std::endl;
    file.seekg(0, std::ios::beg);
    ftruncate(shm_fd, sizeof(SharedData) + SIZE);

    // Map shared memory
    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData)+SIZE,
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    // Initialize semaphore (only needed once)
    
    if (pthread_mutex_init(&shared->lock, NULL) == -1) {
        perror("pthread_mutex_init");
        return 1;
    }
    // Lock the semaphore before writing
    while(true){
        pthread_mutex_lock(&shared->lock);
        shared->wlock++;
        if (shared->wlock > 1 || shared->num_of_reads > 0){
            std::cout<<"WLOCK: " << shared->wlock<< "NUM_OF_READS: "<< shared->num_of_reads << std::endl;
            std::cout<< "WAIT CONDITION"<< std::endl;
            pthread_cond_wait(&shared->writer_cv, &shared->lock);
        }
        pthread_mutex_unlock(&shared->lock);
        shared->filesize = fileSize;

        // Writing data
                                    // Measure write time
        auto start = std::chrono::high_resolution_clock::now();
        file.read(shared->value, fileSize);
        auto end = std::chrono::high_resolution_clock::now();
        pthread_mutex_lock(&shared->lock);
        signal_next(shared);
        shared->wlock--;
        pthread_mutex_unlock(&shared->lock);
        double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
        std::cout << "[Writer] Wrote value: " << "SIZE= "<<SIZE<< std::endl;
        std::cout << "[Writer] Wrote value: " << "SIZE= "<<shared->value<< std::endl;
        std::cout << "Write Speed: " << (SIZE / (elapsed_time / 1e6)) / (1024 * 1024* 1024) << " MB/s" << std::endl;
        std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        
            // Unlock
        std::cout<< "MUTEX OPEN"<< std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    // Cleanup
    munmap(shared, SIZE);
    close(shm_fd);

    return 0;
}
