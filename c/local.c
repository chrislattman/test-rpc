#define _GNU_SOURCE // needed for ftruncate
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <stdlib.h>

int main(void)
{
    char date[64], buf[200] = {0}, *entry;
    const char *name;
    struct stat statbuf;
    struct tm *time_info;
    int fd, i, n;
    ssize_t status;
    struct dirent **namelist;

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
    ftruncate(fd, 30);
    fsync(fd);
    close(fd);

    rename("test_file.txt", "renamed_file.txt");
    unlink("deleted_file.txt");

    n = scandir(".", &namelist, NULL, alphasort);
    for (i = 0; i < n; i++) {
        name = namelist[i]->d_name;
        stat(name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            printf("%s/\n", name);
        }
        if (S_ISREG(statbuf.st_mode)) {
            printf("%s\n", name);
        }
    }
    free(namelist);

    return 0;
}
