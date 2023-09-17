
/* Developed By Hamid.Memar         */
/* License : MIT                    */

#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

using namespace std;

// Global Switch
bool SDKActivated = false;

// Macros
#define SDK_API                     extern "C" _declspec(dllexport)
#define Action_ExecuteCommand       0xEC
#define MaximumCommandBufferSize	100000000
#define EngineLoopInterval			100
#define ValidateSDK                 if (!SDKActivated) return 0

// Command Buffer
struct CommandBuffer
{
	unsigned char commandID = 0;
	unsigned char buffer[MaximumCommandBufferSize - sizeof(unsigned char)] = { 0 };
};

// Shared Memory Objects & Values
HANDLE sharedMemoryHandle   = nullptr;
LPVOID sharedMemoryAddr     = nullptr;

// SDK API
SDK_API bool InitializeRemoteMXSEngineSDK()
{
    // Activate SDK
    SDKActivated = true;
    return SDKActivated;
}
SDK_API int  GetFirst3dsMaxInstanceProcessID()
{
    ValidateSDK;

    // Find 3ds Max Process ID
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);
    if (Process32First(snapshot, &process))
    {
        do
        {
            if (wstring(process.szExeFile) == wstring(L"3dsmax.exe"))
            {
                pid = process.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &process));
    }
    CloseHandle(snapshot);

    return pid;
}
SDK_API bool ValidateRemote3dsMaxConnection(int maxPID)
{
    ValidateSDK;

    wchar_t memoryPathBuffer[128];
    swprintf_s(memoryPathBuffer, 128, L"RemoteMXSConnection_D3E62_%08X", maxPID);
    wstring memoryConnectionPath(memoryPathBuffer);
    HANDLE tempMemoryHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memoryConnectionPath.c_str());
    if (tempMemoryHandle == NULL) return false;

    return CloseHandle(tempMemoryHandle);
}
SDK_API bool InitializeRemote3dsMaxConnection(int maxPID)
{
    ValidateSDK;
    if (sharedMemoryHandle)
    {
        // Already Connected, Function Aborted.
        return false;
    }

    // Generate Memory Path
    wchar_t memoryPathBuffer[128];
    swprintf_s(memoryPathBuffer, 128, L"RemoteMXSConnection_D3E62_%08X", maxPID);
    wstring memoryConnectionPath(memoryPathBuffer);

    // Open Handle to Shared Memory
    sharedMemoryHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memoryConnectionPath.c_str());
    if (sharedMemoryHandle == NULL) return false;
    sharedMemoryAddr = MapViewOfFile(sharedMemoryHandle, FILE_MAP_WRITE, 0, 0, 0);
    if (sharedMemoryAddr == NULL) { CloseHandle(sharedMemoryHandle); return false; }

    return true;
}
SDK_API bool ReleaseRemote3dsMaxConnection()
{
    ValidateSDK;

    if (sharedMemoryAddr) { if (!UnmapViewOfFile(sharedMemoryAddr)) return false; }
    if (sharedMemoryHandle) { if (!CloseHandle(sharedMemoryHandle)) return false; }

    sharedMemoryAddr = nullptr;
    sharedMemoryHandle = nullptr;

    return true;
}
SDK_API bool ExecuteRemoteMaxScript(wchar_t* maxscriptScript)
{
    ValidateSDK;

    // Validate Connection
    if (!sharedMemoryHandle)
    {
        // No Active Remote Connection Found
        return false;
    }
    if (!sharedMemoryAddr)
    {
        // No Active Remote Command Buffer Found
        return false;
    }

    // Create Script Data
    wstring scriptData(maxscriptScript);

    // Get Command Buffer
    CommandBuffer* commandBuffer = (CommandBuffer*)sharedMemoryAddr;

    // Write Data to Command Buffer
    wmemcpy((wchar_t*)commandBuffer->buffer, scriptData.data(), scriptData.size());

    // Mark for Execution
    commandBuffer->commandID = Action_ExecuteCommand;

    // All Good
    return true;
}