#include <Windows.h>
#include "Core/Core.h"
#include "Utils/ExceptionHandler/ExceptionHandler.h"

static bool IsManuallyMapped(HMODULE hModule)
{
	// Modules loaded through LoadLibrary/imports are tracked by the loader; GetModuleHandleEx
	// with FROM_ADDRESS only resolves addresses backed by a loader module, so a hand-mapped
	// image fails this check.
	HMODULE hFound = nullptr;
	return !GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(hModule),
		&hFound);
}

DWORD WINAPI MainThread(LPVOID lpParam)
{
	U::ExceptionHandler.Initialize(lpParam);

	U::Core.Load();
	U::Core.Loop();
	U::Core.Unload();

	U::ExceptionHandler.Unload();

	const auto hModule = static_cast<HMODULE>(lpParam);

	// Manually mapped modules aren't tracked by the loader; FreeLibrary would be invalid there
	// (and some loaders unmap on it), so just end the thread and let the loader clean up.
	if (IsManuallyMapped(hModule))
		ExitThread(EXIT_SUCCESS);
	else
		FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);

	return EXIT_SUCCESS;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (const auto hThread = CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr))
			CloseHandle(hThread);
	}

	return TRUE;
}