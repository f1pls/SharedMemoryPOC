#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <thread>
#include <chrono>
#include "signaler.h"

const char* SHM_NAME = "/shm_mutex_example";
const char* SEM_NAME = "/sem_mutex_example";
const int SIZE=60000000;

int main() {
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    // Set size of shared memory
    // Map shared memory
    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData)+SIZE,
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Lock the semaphore before reading
    char* data = new char[shared->filesize];
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
        while (shared->wlock > 0){
            std::cout<<"WLOCK: " << shared->wlock<< "NUM_OF_READS: "<< shared->num_of_reads << std::endl;
            std::cout<< "WAIT CONDITION"<< std::endl;
            pthread_cond_wait(&shared->reader_cv, &shared->lock);
        }
        shared->num_of_reads++;

        pthread_mutex_unlock(&shared->lock);
        auto start = std::chrono::high_resolution_clock::now();
        data = shared->value;
        auto end = std::chrono::high_resolution_clock::now();

        pthread_mutex_lock(&shared->lock);
        shared->num_of_reads--;
        signal_next(shared);
        pthread_mutex_unlock(&shared->lock);
        size_t fileSize = shared->filesize;
        double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
        if (starttime - now + fps <= std::chrono::seconds{0}){
            fps = fps + std::chrono::seconds{1};
            std::cout << "FPS: " << counter/std::chrono::seconds(1).count()<<std::endl;
            std::cout << "[Reader] Read value: " << shared->value << std::endl;
            std::cout << "[Reader] Read size: " << shared->filesize << std::endl;    
            std::cout << "PNG file read from shared memory and saved as output.png\n";

            counter = 0;
            std::cout << "Read Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024 * 1024) << " MB/s" << std::endl;
            std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        }
    }
    output.write(data, shared->filesize);
    output.close();
    
    // Cleanup
    munmap(shared,SIZE);
    close(shm_fd);

    return 0;
}
