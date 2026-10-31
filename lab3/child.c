#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

struct SharedData {
    char text[1024];
    int length;
};

int format_number(long num, char *buf) {
    int i = 0;
    int neg = 0;
    
    if (num < 0) {
        neg = 1;
        num = -num;
    }
    
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    if (neg) buf[i++] = '-';
    
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    return i;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        write(2, "error args\n", 11);
        return 1;
    }

    char *shm_name = argv[1];
    char *sem_full_name = argv[2];
    char *sem_empty_name = argv[3];

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        write(2, "error shm_open\n", 15);
        exit(1);
    }

    struct SharedData *shared = mmap(NULL, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        write(2, "error mmap\n", 11);
        exit(1);
    }
    close(shm_fd);

    sem_t *sem_full = sem_open(sem_full_name, 0);
    sem_t *sem_empty = sem_open(sem_empty_name, 0);

    char buf[1024];
    ssize_t n;
    
    while ((n = read(0, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        
        char *ptr = buf;
        char *endptr;
        
        while (*ptr != '\0') {
            while (*ptr != '\0' && *ptr != '-' && (*ptr < '0' || *ptr > '9')) {
                ptr++;
            }
            if (*ptr == '\0') break;

            long result = strtol(ptr, &endptr, 10);
            if (ptr == endptr) break;
            ptr = endptr;
            
            while (*ptr == ' ') ptr++;
            
            while (*ptr != '\0' && *ptr != '\n') {
                long divisor = strtol(ptr, &endptr, 10);
                if (ptr == endptr) break;
                
                if (divisor == 0) {
                    sem_wait(sem_empty);
                    
                    char *msg = "devision by zero\n";
                    int len = 0;
                    while (msg[len] != '\0') {
                        shared->text[len] = msg[len];
                        len++;
                    }
                    shared->length = len;
                    
                    sem_post(sem_full);
                    
                    sem_wait(sem_empty);
                    shared->length = 0;
                    sem_post(sem_full);
                    
                    exit(1);
                }
                
                result = result / divisor;
                ptr = endptr;
                while (*ptr == ' ') ptr++;
            }
            
            sem_wait(sem_empty);
            
            int len = format_number(result, shared->text);
            shared->text[len++] = '\n';
            shared->length = len;
            
            sem_post(sem_full);
            
            if (*ptr == '\n') ptr++;
        }
    }
    
    sem_wait(sem_empty);
    shared->length = 0;
    sem_post(sem_full);

    return 0;
}