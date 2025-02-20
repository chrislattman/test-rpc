#include <windows.h>
#include <stdio.h>

void InjectDLL(HANDLE hProcess, const char *dllPath) {
    LPVOID remoteMemory;
    LPTHREAD_START_ROUTINE pLoadLibrary;
    HANDLE hThread;

    // Allocate memory in the child process for the DLL path
    remoteMemory = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMemory) {
        puts("VirtualAllocEx failed");
        return;
    }

    // Write the DLL path into the allocated memory
    WriteProcessMemory(hProcess, remoteMemory, dllPath, strlen(dllPath) + 1, NULL);

    // Get the address of LoadLibraryA in KERNEL32.dll
    pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("KERNEL32.dll"), "LoadLibraryA");

    // Create a remote thread to load the DLL in the child process
    hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, remoteMemory, 0, NULL);
    if (!hThread) {
        puts("CreateRemoteThread failed");
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        return;
    }

    // Wait for the DLL to be loaded and then clean up
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
}

int main(void) {
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    const char *dllPath = "winshim.dll";
    const char *childProcessPath = "local.exe";

    si.cb = sizeof(si);
    // Create the child process in a suspended state
    if (!CreateProcessA(NULL, (LPSTR)childProcessPath, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        puts("CreateProcess failed");
        return 1;
    }

    InjectDLL(pi.hProcess, dllPath);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
