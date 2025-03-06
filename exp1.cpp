#include <iostream>
#include <fcntl.h>     // O_CREAT, O_RDWR
#include <sys/mman.h>  // shm_open, mmap
#include <unistd.h>    // ftruncate, close
#include <semaphore.h> // sem_t
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>

const char* SHM_NAME = "/shm_mutex_example";
const char* SEM_NAME = "/sem_mutex_example";
const int SIZE = 5574;
struct SharedData {
    sem_t mutex;
    size_t filesize;
    char value[5574];
};

int main() {
    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    std::ifstream file("testimage.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    // Set size of shared memory
    size_t fileSize = file.tellg();
    std:: cout<<fileSize<<std::endl;
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
    if (sem_init(&shared->mutex, 1, 1) == -1) {
        perror("sem_init");
        return 1;
    }
    // Lock the semaphore before writing
    int i=0;
    while(true){
        std::cout<< i<< std::endl;
        i++;
        sem_wait(&shared->mutex);
        std::cout<< "MUTEX CLOSED"<< std::endl;
        // Writing data
        auto start = std::chrono::high_resolution_clock::now();
        shared->filesize = fileSize;
        // Measure write time
        file.read(shared->value, fileSize);
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
        std::cout << "[Writer] Wrote value: " << "SIZE= "<<SIZE<< std::endl;
        std::cout << "[Writer] Wrote value: " << "SIZE= "<<shared->value<< std::endl;
        std::cout << "Write Speed: " << (SIZE / (elapsed_time / 1e6)) / (1024 * 1024) << " MB/s" << std::endl;
        std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        sem_post(&shared->mutex); // Unlock
        std::cout<< "MUTEX OPEN"<< std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    // Cleanup
    munmap(shared, SIZE);
    close(shm_fd);

    return 0;
}
