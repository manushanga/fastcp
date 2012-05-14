#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#define BUFFER_SIZE 1024*1024*200
typedef unsigned char uchar;

uchar buffer[BUFFER_SIZE];

struct fds_t{
    int fd_src;
    int fd_dest;
};

sem_t lock;
int end = 0;
void *copy(void *arg){
    struct fds_t *fds = (struct fds_t *)arg;
    
    ssize_t rd = read(fds->fd_src, buffer, BUFFER_SIZE);
    ssize_t wd;
    while (rd != 0){
        if (rd == -1) {
            perror("Read fail");
            exit(0);
        }
        printf("%d\n",rd);
        wd = write(fds->fd_dest, buffer, rd);
        if (wd == -1) {
            perror("Write fail");
            exit(0);
        }
        printf("called fd\n");
        sem_post(&lock);
        printf("returning from fd\n");
        rd = read(fds->fd_src, buffer, BUFFER_SIZE);
    }
    end = 1;
    return NULL;
}
void *sync(void *arg){
    struct fds_t *fds = (struct fds_t *)arg;
    while (1) {
        sem_wait(&lock);
        printf("syncing\n");
        fdatasync(fds->fd_dest);
        if (end == 1)
            break;
    }
    return NULL;
}
int main(int argc, char** argv) {
    struct fds_t fds;
    
    fds.fd_src = open(argv[1], O_RDONLY);
    if (fds.fd_src == -1) {
        perror("Source open");
        exit(0);
    }
    fds.fd_dest = open(argv[2], O_CREAT | O_EXCL | O_WRONLY );
    if (fds.fd_dest == -1) {
        perror("Destination open");
        exit(0);
    }
    sem_init(&lock, 0, 0);
    
    pthread_t th_copy, th_sync;
    pthread_create(&th_copy, NULL, copy, &fds);
    pthread_create(&th_sync, NULL, sync, &fds);
    pthread_join(th_copy, NULL);
    pthread_join(th_sync, NULL);
    return 0;
}

