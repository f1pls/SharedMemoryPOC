#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

const char* SHM_NAME = "/my_shared_memory";

int main() {
    // Open existing shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Get shared memory size
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("fstat");
        return 1;
    }
    size_t filesize = shm_stat.st_size;

    // Map shared memory
    void* ptr = mmap(0, filesize, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Save shared memory content to a new PNG file
    std::ofstream output("output.png", std::ios::binary);
    output.write(static_cast<char*>(ptr), filesize);
    output.close();

    std::cout << "PNG file read from shared memory and saved as output.png\n";

    // Cleanup
    munmap(ptr, filesize);
    close(shm_fd);

    return 0;
}
