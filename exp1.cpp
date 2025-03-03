#include <iostream>
#include <fstream>
#include <sys/mman.h>   // mmap, shm_open
#include <fcntl.h>      // O_* constants
#include <unistd.h>     // ftruncate, close
#include <cstring>      // memcpy
#include <sys/stat.h>   // stat
#include <sys/time.h>  //timestamp
#include <chrono>
#include <thread>

const char* SHM_NAME = "/my_shared_memory";

int main() {
    // Open PNG file in binary mode
    std::ifstream file("testimage.png", std::ios::binary | std::ios::ate);
    std::ifstream file2("testimage2.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    // Get file size
    size_t filesize = file2.tellg() + 2;
    file2.seekg(0, std::ios::beg);

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
    // Write marker (e.g., 0x01 0x00 or 0x10 0x00)
    unsigned char* data = static_cast<unsigned char*>(ptr);
    data[0] = 0x01;  // First byte
    data[1] = 0x00;  // Second byte
    // Read file into memory and copy to shared memory
    while (true){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "FILE 1!\n";
        file.read(reinterpret_cast<char*>(data + 2), filesize);
        pause();
        std::this_thread::sleep_for(std::chrono::milliseconds(50000));
        std::cout << "FILE 2!\n";
        file2.read(static_cast<char*>(ptr), filesize);
    }
    file.close();
    file2.close();
    std::cout << "PNG file loaded into shared memory (" << filesize << " bytes)\n" << "at time: ";

    // Keep the process alive for testing
    std::cout << "Press ENTER to exit...\n";
    std::cin.get();

    // Cleanup
    munmap(ptr, filesize);
    close(shm_fd);
    shm_unlink(SHM_NAME); // Remove shared memory when done

    return 0;
}
