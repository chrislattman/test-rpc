#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

int main(void)
{
    char date[64], buf[200] = {0};
    struct stat statbuf;
    struct tm *time_info;
    int fd;
    ssize_t status;

    stat("test_file.txt", &statbuf);
    printf("File size (in bytes): %ld\n", statbuf.st_size);
    time_info = gmtime(&statbuf.st_mtime);
    strftime(date, sizeof(date), "%a, %d %b %Y %X GMT", time_info);
    printf("Last modified time: %s\n", date);

    fd = open("test_file.txt", O_RDWR);
    status = read(fd, buf, 5);
    printf("%ld %s\n", status, buf);
    lseek(fd, 0, SEEK_SET);
    status = read(fd, buf + 5, 6);
    printf("%ld %s\n", status, buf);
    write(fd, "word", 4);
    fsync(fd);
    close(fd);

    return 0;
}