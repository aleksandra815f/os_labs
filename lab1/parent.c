#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
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
    
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        write(2, "error pipe\n", 11);
        close(file_fd);
        return 1;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        write(2, "error fork\n", 11);
        close(file_fd);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }
    
    if (pid == 0) {
        close(pipe_fd[0]);
        
        if (dup2(file_fd, 0) == -1) {
            write(2, "error file dup2\n", 16);
            exit(1);
        }
        if (dup2(pipe_fd[1], 1) == -1) {
            write(2, "error pipe dup2\n", 16);
            exit(1);
        }
        close(file_fd);
        close(pipe_fd[1]);

        char *args[] = {"./child", NULL};
        if (execvp(args[0], args) == -1) {
            write(2, "error execvp\n", 13);
            exit(1);
        }
    } else {
        close(pipe_fd[1]);
        close(file_fd);
        char buf[1024];
        ssize_t r;
        while ((r = read(pipe_fd[0], buf, sizeof(buf))) > 0) {
            write(1, buf, r);
        }
        if (r == -1) {
            write(2, "error read pipe\n", 16);
        }
        close(pipe_fd[0]);

        if (wait(NULL) == -1) {
            write(2, "error wait\n", 11);
            return 1;
        }
    }
    
    return 0;
}