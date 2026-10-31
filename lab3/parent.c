#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

struct SharedData {
    char text[1024];
    int length;
};

void make_names(pid_t pid, char *shm, char *sem_f, char *sem_e) {
    char p[16];
    int i = 0;
    pid_t temp = pid;
    if (temp == 0) p[i++] = '0';
    else {
        while (temp > 0) { p[i++] = '0' + (temp % 10); temp /= 10; }
    }
    p[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char t = p[j]; p[j] = p[i-1-j]; p[i-1-j] = t;
    }
    
    int idx = 0;
    const char *pref1 = "/shm_";
    for (int j = 0; pref1[j]; j++) shm[idx++] = pref1[j];
    for (int j = 0; p[j]; j++) shm[idx++] = p[j];
    shm[idx] = '\0';

    idx = 0;
    const char *pref2 = "/sem_full_";
    for (int j = 0; pref2[j]; j++) sem_f[idx++] = pref2[j];
    for (int j = 0; p[j]; j++) sem_f[idx++] = p[j];
    sem_f[idx] = '\0';

    idx = 0;
    const char *pref3 = "/sem_empty_";
    for (int j = 0; pref3[j]; j++) sem_e[idx++] = pref3[j];
    for (int j = 0; p[j]; j++) sem_e[idx++] = p[j];
    sem_e[idx] = '\0';
}

int main() {
    char filename[256];
    
    write(1, "Enter filename: ", 16);
    ssize_t n = read(0, filename, sizeof(filename) - 1);
    if (n <= 0) {
        write(2, "error read\n", 11);
        return 1;
    }
    filename[n - 1] = '\0';

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        write(2, "error open\n", 11);
        return 1;
    }

    pid_t my_pid = getpid();
    char shm_name[64], sem_full_name[64], sem_empty_name[64];
    make_names(my_pid, shm_name, sem_full_name, sem_empty_name);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        write(2, "error shm_open\n", 15);
        return 1;
    }
    
    if (ftruncate(shm_fd, sizeof(struct SharedData)) == -1) {
        write(2, "error ftruncate\n", 16);
        return 1;
    }

    struct SharedData *shared = mmap(NULL, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        write(2, "error mmap\n", 11);
        return 1;
    }

    sem_t *sem_full = sem_open(sem_full_name, O_CREAT, 0666, 0);
    sem_t *sem_empty = sem_open(sem_empty_name, O_CREAT, 0666, 1);
    
    if (sem_full == SEM_FAILED || sem_empty == SEM_FAILED) {
        write(2, "error sem_open\n", 15);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        write(2, "error fork\n", 11);
        return 1;
    }

    if (pid == 0) {
        close(shm_fd);
        if (dup2(file_fd, 0) == -1) {
            write(2, "error dup2\n", 11);
            exit(1);
        }
        close(file_fd);

        char *args[] = {"./child", shm_name, sem_full_name, sem_empty_name, NULL};
        if (execv("./child", args) == -1) {
            write(2, "error execv\n", 12);
            sem_wait(sem_empty);
            shared->length = 0;
            sem_post(sem_full);
            exit(1);
        }
    } else {
        close(file_fd);

        while (1) {
            if (sem_wait(sem_full) == -1) break;

            if (shared->length == 0) {
                sem_post(sem_empty);
                break;
            }

            write(1, shared->text, shared->length);
            sem_post(sem_empty);
        }

        wait(NULL);

        munmap(shared, sizeof(struct SharedData));
        close(shm_fd);
        sem_close(sem_full);
        sem_close(sem_empty);
        sem_unlink(sem_full_name);
        sem_unlink(sem_empty_name);
        shm_unlink(shm_name);
    }
    
    return 0;
}