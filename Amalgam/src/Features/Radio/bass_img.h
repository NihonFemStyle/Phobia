#pragma once

#include <Windows.h>

extern const unsigned char bass_dll_image[ 166872 ];

typedef void *HCUSTOMMODULE;

typedef HCUSTOMMODULE ( *MemLoadLibraryFn )( LPCSTR, void * );
typedef FARPROC ( *MemGetProcAddressFn )( HANDLE, LPCSTR, void * );
typedef void ( *MemFreeLibraryFn )( HANDLE, void * );

typedef BOOL( WINAPI *DllEntryProc )( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved );
typedef int( WINAPI *ExeEntryProc )( void );

typedef struct
{
    PIMAGE_NT_HEADERS headers;
    unsigned char *codeBase;
    HCUSTOMMODULE *modules;
    int numModules;
    BOOL initialized;
    BOOL isDLL;
    BOOL isRelocated;
    MemLoadLibraryFn loadLibrary;
    MemGetProcAddressFn getProcAddress;
    MemFreeLibraryFn freeLibrary;
    void *userdata;
    ExeEntryProc exeEntry;
    DWORD pageSize;
} MEMORYMODULE, *PMEMORYMODULE;

typedef struct
{
    LPVOID address;
    LPVOID alignedAddress;
    DWORD size;
    DWORD characteristics;
    BOOL last;
} SECTIONFINALIZEDATA, *PSECTIONFINALIZEDATA;

class CWin32PE
{
protected:
    BOOL CheckSize( size_t size, size_t expected );
    DWORD GetRealSectionSize( PMEMORYMODULE module, PIMAGE_SECTION_HEADER section );
    BOOL CopySections( const unsigned char *data, size_t size, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module );
    BOOL FinalizeSection( PMEMORYMODULE module, PSECTIONFINALIZEDATA sectionData );
    BOOL FinalizeSections( PMEMORYMODULE module );
    BOOL ExecuteTLS( PMEMORYMODULE module );
    BOOL PerformBaseRelocation( PMEMORYMODULE module, ptrdiff_t delta );
    BOOL BuildImportTable( PMEMORYMODULE module );
};

class CDllModule : protected CWin32PE
{
public:
    HANDLE LoadFromMemory( PVOID module, SIZE_T size );
    HANDLE LoadFromResources( int IDD_RESOUCE );
    HANDLE LoadFromFile( LPCSTR filename );

    FARPROC GetProcAddressFromMemory( HANDLE hModule, LPCSTR ProcName );

    int CallEntryPointFromMemory( HANDLE hModule );
    void FreeLibraryFromMemory( HANDLE hModule );

private:
    HANDLE MemLoadLibraryEx( const void *data, size_t size, MemLoadLibraryFn loadLibrary, MemGetProcAddressFn getProcAddress, MemFreeLibraryFn freeLibrary, void *userdata );
};

namespace BASS
{
    inline CDllModule bass_lib;
    inline HANDLE bass_lib_handle;
    inline DWORD stream_handle;
    inline uintptr_t request_num;
    inline BOOL bass_init;
    inline char bass_metadata[ MAX_PATH ];
    inline char bass_channelinfo[ MAX_PATH ];
} // namespace RadioVars