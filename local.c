#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    char buf[200] = {0};
    int fd;
    ssize_t status;

    fd = open("test_file.txt", O_RDWR);
    status = read(fd, buf, 5);
    printf("%ld %s\n", status, buf);
    lseek(fd, 0, SEEK_SET);
    status = read(fd, buf + 5, 6);
    printf("%ld %s\n", status, buf);
    status = write(fd, "word", 4);
    close(fd);

    return 0;
}
