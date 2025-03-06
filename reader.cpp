#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <thread>
#include <chrono>

const char* SHM_NAME = "/shm_mutex_example";
const char* SEM_NAME = "/sem_mutex_example";
const int SIZE=5574;

struct SharedData {
    sem_t mutex;
    size_t filesize;
    char value[5574];
};

int main() {
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
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
    // Map shared memory
    SharedData* shared = (SharedData*)mmap(nullptr, sizeof(SharedData)+SIZE,
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Lock the semaphore before reading
    int i=0;
    while(true){
        std::cout<< i<< std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(7000));
        i++;
        sem_wait(&shared->mutex);
        std::cout<< "MUTEX CLOSED"<< std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        size_t fileSize = shared->filesize;
        auto start = std::chrono::high_resolution_clock::now();
        std::ofstream output("output.png", std::ios::binary);
        output.write(shared->value, fileSize);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "[Reader] Read value: " << shared->value << std::endl;
        std::cout << "[Reader] Read size: " << shared->filesize << std::endl;    
        std::cout << "PNG file read from shared memory and saved as output.png\n";
        double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
        std::cout << "Read Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024) << " MB/s" << std::endl;
        std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
        output.close();
        sem_post(&shared->mutex); // Unlock
        std::cout<< "MUTEX OPEN"<< std::endl;
    }
    // Cleanup
    munmap(shared, SIZE);
    close(shm_fd);

    return 0;
}
