#include <fcntl.h>
#include <sys/mman.h>

const char* SHM_NAME = "/my_shared_memory";

int main() {
    shm_unlink(SHM_NAME);
    return 0;
}
