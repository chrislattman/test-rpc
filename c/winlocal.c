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
    WIN32_FIND_DATA ffd;

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

    // equivalent to truncating the file to 30 bytes
    SetFilePointer(file, 30, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    FlushFileBuffers(file);
    CloseHandle(file);

    MoveFileA("test_file.txt", "renamed_file.txt");
    DeleteFileA("deleted_file.txt");

    file = FindFirstFileA(".\\*", &ffd);
    if (file != INVALID_HANDLE_VALUE) {
        do {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                printf("%s/\n", ffd.cFileName);
            }
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) { // for some reason it's not FILE_ATTRIBUTE_NORMAL
                printf("%s\n", ffd.cFileName);
            }
        } while (FindNextFileA(file, &ffd));
        FindClose(file);
    }

    return 0;
}
