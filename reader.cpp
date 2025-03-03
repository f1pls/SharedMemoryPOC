#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

const char* SHM_NAME = "/my_shared_memory";

int main() {
    // Open existing shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Get shared memory size
    // 
    // WHY NOT PASS THE FILE SIZE IN FIRST 6
    // next 6 HH:MM:SS 13:06:30
    // upon getting image consumer sends back timestamp of receive point and converted to 16th numerical size 
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
    unsigned char* data = static_cast<unsigned char*>(ptr);
    std::cout << "Marker: " << std::hex << static_cast<int>(data[0]) << " " << static_cast<int>(data[1]) << std::dec << std::endl;
    std::cout <<"data: " << reinterpret_cast<char*>(data) <<std::endl;
    // for (int i=0; i <= sizeof(data); i++){
    //     if (i+2 <= sizeof(data)){ 
    //         data[i]= data[i+2];
    //     }
    //     else{
    //         break;
    //     }
    // }
    // Save shared memory content to a new PNG file
    // std::ofstream output("output.png", std::ios::binary);
    // output.write(reinterpret_cast<char*>(data-2), filesize-2);
    output.close();

    std::cout << "PNG file read from shared memory and saved as output.png\n";

    // Cleanup
    munmap(ptr, filesize);
    close(shm_fd);

    return 0;
}
