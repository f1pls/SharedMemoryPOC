#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>

const char* SHM_NAME = "/shm_perf";

int main() {
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Get size of shared memory
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("fstat");
        return 1;
    }
    size_t shm_size = shm_stat.st_size;

    // Map shared memory
    void* ptr = mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Read marker (first two bytes)
    unsigned char* data = static_cast<unsigned char*>(ptr);
    std::cout << "Marker: " << std::hex << static_cast<int>(data[0]) << " " << static_cast<int>(data[1]) << std::dec << std::endl;
    size_t image_size = shm_size;
    std::ofstream output("output.png", std::ios::binary);
    std::cout << "PNG file read from shared memory and saved as output.png\n";
    output.write(reinterpret_cast<char*>(data), image_size);
    output.close();
    // Cleanup
    munmap(ptr, shm_size);
    close(shm_fd);

    return 0;
}
