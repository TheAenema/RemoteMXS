
/* Developed By Hamid.Memar         */
/* License : MIT                    */

#include <maxscript/maxscript.h>
#include <maxapi.h>
#include <CoreFunctions.h>
#include <notify.h>
#include <string>
#include <Windows.h>

using namespace std;

// Instances
HINSTANCE hInstance;

// Plugin Entrypoint
BOOL WINAPI DllMain(HINSTANCE instance, ULONG reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		hInstance = instance;
		DisableThreadLibraryCalls(hInstance);
	}
	return(TRUE);
}

// Macros
#define MaximumCommandBufferSize	100000000
#define EngineLoopInterval			50
#define Action_ExecuteCommand       0xEC
#define WM_TRIGGER_CALLBACK			WM_USER + 4764

// Command Buffer
struct CommandBuffer
{
	unsigned char commandID = 0;
	unsigned char buffer[MaximumCommandBufferSize - sizeof(unsigned char)] = { 0 };
};

// Shared Memory Objects & Values
wstring sharedMemoryPath;
HANDLE sharedMemoryHandle = nullptr;
LPVOID sharedMemoryAddr = nullptr;

// Shared Memory Functions
wstring GenerateSharedMemoryPath()
{
	wchar_t memoryPathBuffer[128];
	swprintf_s(memoryPathBuffer, 128, L"RemoteMXSConnection_D3E62_%08X", GetCurrentProcessId());
	return wstring(memoryPathBuffer);
}
bool InitializeSharedMemory()
{
	sharedMemoryHandle = CreateFileMapping (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MaximumCommandBufferSize, sharedMemoryPath.c_str());
	if (sharedMemoryHandle == NULL) return false;
	sharedMemoryAddr = MapViewOfFile(sharedMemoryHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (sharedMemoryAddr == NULL) 
	{
		CloseHandle(sharedMemoryHandle);
		return false;
	}
	return true;
}
bool ReleaseSharedMemory()
{
	if (sharedMemoryAddr) { if (!UnmapViewOfFile(sharedMemoryAddr)) return false; }
	if (sharedMemoryHandle) { if (!CloseHandle(sharedMemoryHandle)) return false; }
	return true;
}

// Executors
wstring currentMaxscriptScript = L"";
void FN_ExecuteMaxScript()
{
	// Execute Script With Restrictions
	ExecuteMAXScriptScript(currentMaxscriptScript.c_str(), MAXScript::ScriptSource::Dynamic, TRUE, NULL, TRUE);

	// Execute Script Without Restrictions
	/* ExecuteMAXScriptScript(currentMaxscriptScript.c_str(), MAXScript::ScriptSource::NonEmbedded, TRUE, NULL, TRUE); */

	// Clear Script Data
	currentMaxscriptScript = L"";
}

// Remote Engine Loop
bool runEngine = false;
HANDLE engineLoopThread = nullptr;
void EngineLoop()
{
	while (runEngine)
	{
		// Skip If Buffer is Not Valid
		if (!sharedMemoryAddr) continue;

		// Grab Command Buffer And Execute It
		CommandBuffer* commandBuffer = (CommandBuffer*)sharedMemoryAddr;

		// Check for Command Type
		if (commandBuffer->commandID == Action_ExecuteCommand) // Execute Command
		{
			// Execute MaxScript In Main Thread
			currentMaxscriptScript = wstring((wchar_t*)commandBuffer->buffer);
			PostMessage(GetCOREInterface()->GetMAXHWnd(), WM_TRIGGER_CALLBACK, (UINT_PTR)FN_ExecuteMaxScript, 0);

			// Clear The Command Buffer
			memset(sharedMemoryAddr, NULL, MaximumCommandBufferSize);
		}

		// Engine Interval
		Sleep(EngineLoopInterval);
	}
}

// Events
void OnMaxStartup(void* param, NotifyInfo* info) 
{
	// Generate Shared Memory Path
	sharedMemoryPath = GenerateSharedMemoryPath();

	// Initialize Connection
	if (!InitializeSharedMemory())
	{
		MessageBoxA(GetCOREInterface()->GetMAXHWnd(), "Failed to Initialize Inter-Process Connection", "RemoteMXS :: Fatal Error", MB_ICONERROR);
	}

	// Start Engine Loop
	runEngine = true;
	engineLoopThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EngineLoop, NULL, NULL, NULL);
}
void OnMaxShutdown(void* param, NotifyInfo* info) 
{
	// Stop Engine Loop
	runEngine = false;

	// Release Connection
	if (!ReleaseSharedMemory())
	{
		MessageBoxA(GetCOREInterface()->GetMAXHWnd(), "Failed to Release Inter-Process Connection", "RemoteMXS :: Fatal Error", MB_ICONERROR);
	}

	// Close Thread Handle
	CloseHandle(engineLoopThread);
}

// MaxSDK Functions
extern "C" __declspec(dllexport) const TCHAR* LibDescription()
{
	return L"A Thread-Safe Remote MaxScript Engine for Autodesk 3ds Max, Developed By Hamid.Memar";
}
extern "C" __declspec(dllexport) int LibNumberClasses()
{
	return 0;
}
extern "C" __declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
	return 0;
}
extern "C" __declspec(dllexport) ULONG LibVersion()
{
	return Get3DSMAXVersion();
}
extern "C" __declspec(dllexport) int LibInitialize(void)
{
	// Register Events
	RegisterNotification(OnMaxStartup, NULL, NOTIFY_SYSTEM_STARTUP);
	RegisterNotification(OnMaxShutdown, NULL, NOTIFY_SYSTEM_SHUTDOWN);

	return TRUE;
}
extern "C" __declspec(dllexport) int LibShutdown(void)
{
	return TRUE;
}
extern "C" __declspec(dllexport) ULONG CanAutoDefer()
{
	return FALSE;
}