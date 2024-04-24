#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <Windows.h>

int main(void)
{
    char date[64], buf[200] = {0};
    struct stat statbuf;
    struct tm *time_info;
    HANDLE file;
    DWORD status;

    stat("test_file.txt", &statbuf); // Win32 alternative: GetFileSize
    printf("File size (in bytes): %ld\n", statbuf.st_size);
    time_info = gmtime(&statbuf.st_mtime); // Win32 alternative: GetFileTime
    strftime(date, sizeof(date), "%a, %d %b %Y %X GMT", time_info); // Win32 alternative: FileTimeToSystemTime
    printf("Last modified time: %s\n", date);

    file = CreateFileA("test_file.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ReadFile(file, buf, 5, &status, NULL);
    printf("%lu %s\n", status, buf);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    ReadFile(file, buf + 5, 6, &status, NULL);
    printf("%lu %s\n", status, buf);
    WriteFile(file, "word", 4, &status, NULL);
    FlushFileBuffers(file);
    CloseHandle(file);

    MoveFileA("test_file.txt", "renamed_file.txt");
    DeleteFileA("deleted_file.txt");

    return 0;
}
