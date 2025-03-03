//stole all this from gpt, as a test good enuf
#include <iostream>
#include <fstream>
#include <sys/mman.h>   // mmap, shm_open
#include <fcntl.h>      // O_* constants
#include <unistd.h>     // ftruncate, close
#include <cstring>      // memcpy
#include <sys/stat.h>   // stat

const char* SHM_NAME = "/my_shared_memory";

int main() {
    // Open PNG file in binary mode
    std::ifstream file("testimage.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }

    // Get file size
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Resize shared memory to fit the image
    if (ftruncate(shm_fd, filesize) == -1) {
        perror("ftruncate");
        return 1;
    }

    // Map shared memory
    void* ptr = mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Read file into memory and copy to shared memory
    file.read(static_cast<char*>(ptr), filesize);
    file.close();

    std::cout << "PNG file loaded into shared memory (" << filesize << " bytes)\n";

    // Keep the process alive for testing
    std::cout << "Press ENTER to exit...\n";
    std::cin.get();

    // Cleanup
    munmap(ptr, filesize);
    close(shm_fd);
    shm_unlink(SHM_NAME); // Remove shared memory when done

    return 0;
}
