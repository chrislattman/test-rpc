#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

static CreateFileA_t originalCreateFileA = NULL;

// Custom function to replace CreateFileA
HANDLE WINAPI HookedCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    puts("Hooked CreateFileA!");
    return originalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
}

// Function to modify IAT in the child process
void HookIAT(HMODULE hModule, LPCSTR targetFunction, LPVOID hookFunction) {
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);
    IMAGE_DATA_DIRECTORY importDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + importDir.VirtualAddress);
    char *moduleName;
    PIMAGE_THUNK_DATA pThunk;
    FARPROC *pFunction;
    DWORD oldProtect;

    // Iterate over import descriptors (DLLs being imported)
    while (pImportDesc->Name) {
        moduleName = (char *)hModule + pImportDesc->Name;
        if (strcmp(moduleName, "KERNEL32.dll") == 0) {
            pThunk = (PIMAGE_THUNK_DATA)((BYTE*)hModule + pImportDesc->FirstThunk);
            // Iterate over imported functions
            while (pThunk->u1.Function) {
                pFunction = (FARPROC*)&pThunk->u1.Function;
                if (*pFunction == (FARPROC)GetProcAddress(GetModuleHandleA("KERNEL32.dll"), targetFunction)) {
                    // Disable memory protection, store original function pointer, overwrite IAT entry,
                    // and finally restore memory protection
                    VirtualProtect(pFunction, sizeof(FARPROC), PAGE_EXECUTE_READWRITE, &oldProtect);
                    originalCreateFileA = (CreateFileA_t)(*pFunction);
                    *pFunction = (FARPROC)hookFunction;
                    VirtualProtect(pFunction, sizeof(FARPROC), oldProtect, &oldProtect);
                    return; // you can hook more functions in this same if-elseif-else statement
                }
                pThunk++;
            }
        }
        pImportDesc++;
    }
}

// DLL Entry Point
BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        HookIAT(GetModuleHandleA(NULL), "CreateFileA", HookedCreateFileA);
    }
    return TRUE;
}
