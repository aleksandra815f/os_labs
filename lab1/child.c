#include <unistd.h>
#include <stdlib.h>

void write_number(long num) {
    char buf[32];
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
    
    write(1, buf, i);
}

int main() {
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
            if (ptr == endptr){
                break;
            }
            ptr = endptr;
            
            while (*ptr == ' '){
                ptr++;
            }
            while (*ptr != '\0' && *ptr != '\n') {
                long divisor = strtol(ptr, &endptr, 10);
                if (ptr == endptr){
                    break;
                }                
                if (divisor == 0) {
                    write(1, "devision by zero\n", 17);
                    exit(1);
                }
                
                result = result / divisor;
                ptr = endptr;
                while (*ptr == ' ') ptr++;
            }
            
            write_number(result);
            write(1, "\n", 1);
            
            if (*ptr == '\n') ptr++;
        }
    }
    
    if (n == -1) {
        write(2, "error read\n", 11);
        exit(1);
    }
    
    return 0;
}