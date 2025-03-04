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

const char* SHM_NAME = "/shm_png";

int main() {
    // Open PNG file
    std::ifstream file("testimage.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    std::ifstream file2("testimage2.png", std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open image.png\n";
        return 1;
    }
    // Get file size and total shared memory size (image size + 2 bytes for marker)
    size_t image_size = file2.tellg();
    size_t shm_size = image_size + 10;
    file.seekg(0, std::ios::beg);

    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Resize shared memory
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        return 1;
    }

    // Map shared memory
    void* ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    // Write marker
    unsigned char* data = static_cast<unsigned char*>(ptr);
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm ltime;
    localtime_r(&t, &ltime);
    data[0] = 0x01;  // First byte
    data[1] = 0x00;  // Second byte
    data[2] = 0xFF;
    data[3] = 0xFF;
    data[4] = 0xFF; 
    while (true){
        // Read file into memory after the marker
        file.read(reinterpret_cast<char*>(data + 5), image_size);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        file2.read(reinterpret_cast<char*>(data + 5), image_size);
    }
    file.close();
    std::cout << "PNG file written to shared memory with marker (" << image_size << " bytes total)\n";

    // Keep process alive for testing
    std::cout << "Press ENTER to exit...\n";
    std::cin.get();

    // Cleanup
    munmap(ptr, shm_size);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

int decConv(int decimal){ //range 0-255
    std::stringstream hexstr;
    hexstr << std::hex << decimal;
    //have no idea how to convert st to int, std::stoi?
}