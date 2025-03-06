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
const int SIZE=25165824;

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
int main() {
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
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
    while(true){
    while (shared->wlock > 0){
        pthread_cond_wait(&shared->reader_cv, &shared->lock);
    }
    shared->num_of_reads++;
    std::cout<< shared->num_of_reads << std::endl;
    std::cout<< "MUTEX CLOSED"<< std::endl;

    pthread_mutex_unlock(&shared->lock);
    std::cout<< "MUTEX OPEN"<< std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto start = std::chrono::high_resolution_clock::now();
    data = shared->value;
    auto end = std::chrono::high_resolution_clock::now();

    pthread_mutex_lock(&shared->lock);
    shared->num_of_reads--;
    signal_next(shared);
    pthread_mutex_unlock(&shared->lock);
    size_t fileSize = shared->filesize;

    std::cout << "[Reader] Read value: " << shared->value << std::endl;
    std::cout << "[Reader] Read size: " << shared->filesize << std::endl;    
    std::cout << "PNG file read from shared memory and saved as output.png\n";

    double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();
    std::cout << "Read Speed: " << (fileSize / (elapsed_time / 1e6)) / (1024 * 1024 * 1024) << " MB/s" << std::endl;
    std::cout << "elapsed_time: " << elapsed_time << "ms" << std::endl;
    
    output.write(data, fileSize);
    }
    output.close();
    
    // Cleanup
    munmap(shared, SIZE);
    close(shm_fd);

    return 0;
}
