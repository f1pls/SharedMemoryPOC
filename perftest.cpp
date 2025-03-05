#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <bitset>
#include <sstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>

const char* SHM_NAME = "/shm_perf";

int main() {

    std::ifstream file("testimage2.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    size_t SIZE = file.tellg();
    file.seekg(0, std::ios::beg);
    // Get file size and total shared memory size (image size + 2 bytes for marker)
    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Resize shared memory
    if (ftruncate(shm_fd, SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    // Map shared memory
    void* ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Create test data
    char* data = new char[SIZE];
    file.read(reinterpret_cast<char*>(data), SIZE);
    // Measure write time
    auto start = std::chrono::high_resolution_clock::now();
    memcpy(ptr, data, SIZE);  // Copy data to shared memory
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << data[0]<<std::endl;

    // Calculate elapsed time in microseconds
    double elapsed_time = std::chrono::duration<double, std::micro>(end - start).count();

    std::cout << "Write Speed: " << (SIZE / (elapsed_time / 1e6)) / (1024 * 1024) << " MB/s" << std::endl;
    
    char a;
    std::cin.get();
    // Cleanup
    delete[] data;
    munmap(ptr, SIZE);
    close(shm_fd);

    return 0;
}
