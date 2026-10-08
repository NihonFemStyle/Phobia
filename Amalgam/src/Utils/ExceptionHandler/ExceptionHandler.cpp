#include "ExceptionHandler.h"

#include "../../Features/Configs/Configs.h"

#include <ImageHlp.h>
#include <Psapi.h>
#include <winhttp.h>
#include <atomic>
#include <cstring>
#include <deque>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <format>
#pragma comment(lib, "imagehlp.lib")

#define STATUS_RUNTIME_ERROR             ((DWORD   )0xE06D7363L)
#define DBG_THREAD_NAMING                ((DWORD   )0x406D1388L)

struct Frame_t
{
	std::string m_sModule = "";
	uintptr_t m_uBase = 0;
	uintptr_t m_uAddress = 0;
	std::string m_sFile = "";
	unsigned int m_uLine = 0;
	std::string m_sName = "";
};

static PVOID s_pHandle;
static LPVOID s_lpParam;
static int s_iExceptions = 0;
static uintptr_t s_uModuleBase = 0;
static uintptr_t s_uModuleSize = 0;

static std::string ResolveAddress(uintptr_t uAddress)
{
	if (s_uModuleBase && uAddress >= s_uModuleBase && uAddress < s_uModuleBase + s_uModuleSize)
		return std::format("Phobia+{:#x}", uAddress - s_uModuleBase);

	HMODULE hModule;
	if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(uAddress), &hModule))
	{
		uintptr_t uBase = uintptr_t(hModule);
		char buffer[MAX_PATH];
		if (GetModuleBaseNameA(GetCurrentProcess(), hModule, buffer, sizeof(buffer) / sizeof(char)))
			return std::format("{}+{:#x}", buffer, uAddress - uBase);
	}
	return std::format("{:#x}", uAddress);
}

#define CRASH_WEBHOOK_HOST "discord.com"
static constexpr unsigned char s_ucWebhookXOR[] = {
	0x24,0x01,0x1A,0x11,0x01,0x53,0x5C,0x79,0x49,0x2A,0x01,0x02,0x1C,0x1A,0x49,0x7C,0x06,0x1F,0x02,0x5D,0x15,0x5D,0x22,0x4A,0x0E,0x29,0x17,0x06,0x0E,0x1D,0x02,0x00,0x79,0x1C,0x72,0x4B,0x55,0x4A,0x5E,0x1A,0x60,0x55,0x45,0x59,0x4A,0x47,0x1B,0x7D,0x52,0x49,0x74,0x41,0x41,0x37,0x03,0x01,0x26,0x07,0x44,0x01,0x11,0x09,0x3F,0x22,0x7F,0x2B,0x21,0x5D,0x22,0x1E,0x24,0x75,0x78,0x29,0x3F,0x61,0x10,0x43,0x0C,0x15,0x22,0x27,0x1D,0x7E,0x00,0x07,0x03,0x07,0x12,0x72,0x35,0x36,0x11,0x09,0x16,0x42,0x5C,0x7D,0x2A,0x4E,0x78,0x21,0x5D,0x58,0x3E,0x44,0x12,0x3C,0x5F,0x1A,0x1D,0x13,0x16,0x32,0x6A,0x19,0x30,0x38,0x30,0x05,0x2E
};
static inline std::string DecodeWebhook()
{
	constexpr const char* sKey = "Phobia-Crash-Report-Key";
	const size_t uKeyLen = strlen(sKey);
	std::string sOut;
	sOut.reserve(sizeof(s_ucWebhookXOR));
	for (size_t i = 0; i < sizeof(s_ucWebhookXOR); i++)
		sOut += char(s_ucWebhookXOR[i] ^ sKey[i % uKeyLen]);
	return sOut;
}

// winhttp.dll loaded at runtime to avoid linking winhttp.lib
struct CrashWebhookApi
{
	HMODULE hMod = nullptr;

	decltype(&WinHttpOpen) Open = nullptr;
	decltype(&WinHttpConnect) Connect = nullptr;
	decltype(&WinHttpOpenRequest) OpenRequest = nullptr;
	decltype(&WinHttpSendRequest) SendRequest = nullptr;
	decltype(&WinHttpReceiveResponse) ReceiveResponse = nullptr;
	decltype(&WinHttpCloseHandle) CloseHandle = nullptr;

	bool Load()
	{
		if (hMod)
			return Open != nullptr;

		hMod = LoadLibraryA("winhttp.dll");
		if (!hMod)
			return false;

		Open = reinterpret_cast<decltype(&WinHttpOpen)>(GetProcAddress(hMod, "WinHttpOpen"));
		Connect = reinterpret_cast<decltype(&WinHttpConnect)>(GetProcAddress(hMod, "WinHttpConnect"));
		OpenRequest = reinterpret_cast<decltype(&WinHttpOpenRequest)>(GetProcAddress(hMod, "WinHttpOpenRequest"));
		SendRequest = reinterpret_cast<decltype(&WinHttpSendRequest)>(GetProcAddress(hMod, "WinHttpSendRequest"));
		ReceiveResponse = reinterpret_cast<decltype(&WinHttpReceiveResponse)>(GetProcAddress(hMod, "WinHttpReceiveResponse"));
		CloseHandle = reinterpret_cast<decltype(&WinHttpCloseHandle)>(GetProcAddress(hMod, "WinHttpCloseHandle"));

		return Open && Connect && OpenRequest && SendRequest && ReceiveResponse && CloseHandle;
	}
};

static bool UploadCrashLog(const std::string& sLog)
{
	static CrashWebhookApi sApi;
	if (!sApi.Load())
		return false;

	const std::string sURL = DecodeWebhook();
	size_t iScheme = sURL.find("://");
	size_t iSlash = sURL.find('/', iScheme == std::string::npos ? 0 : iScheme + 3);
	const std::string sPath = iSlash == std::string::npos ? "/" : sURL.substr(iSlash);
	const std::wstring wsPath(sPath.begin(), sPath.end());

	const char* sBoundary = "----PhobiaBoundary";
	const std::string sBnd = sBoundary;
	std::string sBody = "--" + sBnd + "\r\n"
		"Content-Disposition: form-data; name=\"content\"\r\n\r\n"
		"New Phobia crash report\r\n"
		"--" + sBnd + "\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"crash_log.txt\"\r\n"
		"Content-Type: text/plain\r\n\r\n"
		+ sLog + "\r\n--" + sBnd + "--\r\n";

	const std::wstring wsHost(CRASH_WEBHOOK_HOST, CRASH_WEBHOOK_HOST + strlen(CRASH_WEBHOOK_HOST));

	HINTERNET hSession = sApi.Open(L"Phobia", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return false;

	bool bSent = false;
	for (int iAttempt = 0; iAttempt < 2 && !bSent; iAttempt++)
	{
		HINTERNET hConnect = sApi.Connect(hSession, wsHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (!hConnect)
			break;

		HINTERNET hRequest = sApi.OpenRequest(hConnect, L"POST", wsPath.c_str(), nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE);
		if (!hRequest)
		{
			sApi.CloseHandle(hConnect);
			break;
		}

		const std::wstring wsHeaders = L"Content-Type: multipart/form-data; boundary=" + std::wstring(sBoundary, sBoundary + sBnd.size()) + L"\r\n";

		bSent = sApi.SendRequest(hRequest, wsHeaders.c_str(), (DWORD)wsHeaders.size(), (LPVOID)sBody.data(), (DWORD)sBody.size(), (DWORD)sBody.size(), 0) != FALSE;
		if (bSent)
			bSent = sApi.ReceiveResponse(hRequest, nullptr) != FALSE;

		sApi.CloseHandle(hRequest);
		sApi.CloseHandle(hConnect);

		if (!bSent)
			::Sleep(700);
	}

	sApi.CloseHandle(hSession);
	return bSent;
}

static inline std::deque<Frame_t> StackTrace(PCONTEXT pContext)
{
	std::deque<Frame_t> vTrace = {};

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

	if (!SymInitialize(hProcess, nullptr, TRUE))
		return vTrace;

	SymSetOptions(SYMOPT_LOAD_LINES);

	STACKFRAME64 tStackFrame = {};
	tStackFrame.AddrPC.Offset = pContext->Rip;
	tStackFrame.AddrFrame.Offset = pContext->Rbp;
	tStackFrame.AddrStack.Offset = pContext->Rsp;
	tStackFrame.AddrPC.Mode = AddrModeFlat;
	tStackFrame.AddrFrame.Mode = AddrModeFlat;
	tStackFrame.AddrStack.Mode = AddrModeFlat;

	CONTEXT tContext = *pContext;

	while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &tStackFrame, &tContext, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
	{
		vTrace.push_back({ .m_uAddress = tStackFrame.AddrPC.Offset });
		Frame_t& tFrame = vTrace.back();

		if (auto hBase = HINSTANCE(SymGetModuleBase64(hProcess, tStackFrame.AddrPC.Offset)))
		{
			tFrame.m_uBase = uintptr_t(hBase);

			char buffer[MAX_PATH];
			if (GetModuleBaseName(hProcess, hBase, buffer, sizeof(buffer) / sizeof(char)))
				tFrame.m_sModule = buffer;
			else
				tFrame.m_sModule = std::format("{:#x}", tFrame.m_uBase);
		}

		{
			DWORD dwOffset = 0;
			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddr64(hProcess, tStackFrame.AddrPC.Offset, &dwOffset, &line))
			{
				tFrame.m_sFile = line.FileName;
				tFrame.m_uLine = line.LineNumber;
				auto iFind = tFrame.m_sFile.rfind("\\");
				if (iFind != std::string::npos)
					tFrame.m_sFile.replace(0, iFind + 1, "");
			}
		}

		{
			DWORD64 dwOffset = 0;
			char buf[sizeof(IMAGEHLP_SYMBOL64) + 255];
			auto symbol = PIMAGEHLP_SYMBOL64(buf);
			symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64) + 255;
			symbol->MaxNameLength = 254;
			if (SymGetSymFromAddr64(hProcess, tStackFrame.AddrPC.Offset, &dwOffset, symbol))
				tFrame.m_sName = symbol->Name;
		}
	}

	SymCleanup(hProcess);

	return vTrace;
}

static bool IsFatalCode(DWORD dwCode)
{
	switch (dwCode)
	{
	case STATUS_ACCESS_VIOLATION:
	case STATUS_IN_PAGE_ERROR:
	case STATUS_ILLEGAL_INSTRUCTION:
	case STATUS_PRIVILEGED_INSTRUCTION:
	case STATUS_ARRAY_BOUNDS_EXCEEDED:
	case STATUS_FLOAT_DENORMAL_OPERAND:
	case STATUS_FLOAT_DIVIDE_BY_ZERO:
	case STATUS_FLOAT_INEXACT_RESULT:
	case STATUS_FLOAT_INVALID_OPERATION:
	case STATUS_FLOAT_OVERFLOW:
	case STATUS_FLOAT_STACK_CHECK:
	case STATUS_FLOAT_UNDERFLOW:
	case STATUS_INTEGER_DIVIDE_BY_ZERO:
	case STATUS_INTEGER_OVERFLOW:
	case STATUS_STACK_OVERFLOW:
	case STATUS_HEAP_CORRUPTION:
	case STATUS_STACK_BUFFER_OVERRUN:
	case 0xC000041D: // STATUS_FATAL_USER_CALLBACK_EXCEPTION
	case STATUS_GUARD_PAGE_VIOLATION:
	case STATUS_DATATYPE_MISALIGNMENT:
	case STATUS_BREAKPOINT:
		return true;
	default:
		return false;
	}
}

static LONG APIENTRY ExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo)
{
	if (!IsFatalCode(ExceptionInfo->ExceptionRecord->ExceptionCode))
		return EXCEPTION_CONTINUE_SEARCH;

	const char* sError = "UNKNOWN";
	switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
	case STATUS_ACCESS_VIOLATION: sError = "ACCESS VIOLATION"; break;
	case STATUS_STACK_OVERFLOW: sError = "STACK OVERFLOW"; break;
	case STATUS_HEAP_CORRUPTION: sError = "HEAP CORRUPTION"; break;
	case STATUS_DATATYPE_MISALIGNMENT: sError = "DATATYPE MISALIGNMENT"; break;
	case STATUS_GUARD_PAGE_VIOLATION: sError = "GUARD PAGE VIOLATION"; break;
	case STATUS_ILLEGAL_INSTRUCTION: sError = "ILLEGAL INSTRUCTION"; break;
	case STATUS_PRIVILEGED_INSTRUCTION: sError = "PRIVILEGED INSTRUCTION"; break;
	case STATUS_INTEGER_DIVIDE_BY_ZERO: sError = "INTEGER DIVIDE BY ZERO"; break;
	}

	if (!Vars::Debug::CrashLogging.Value)
		return EXCEPTION_CONTINUE_SEARCH;

	std::stringstream ssErrorStream;
	ssErrorStream << std::format("Error: {} (0x{:X}) ({})\n", sError, ExceptionInfo->ExceptionRecord->ExceptionCode, ++s_iExceptions);
	ssErrorStream << "Built @ " __DATE__ ", " __TIME__ ", " __CONFIGURATION__ "\n";
	ssErrorStream << std::format("Time @ {}, {}\n", SDK::GetDate(), SDK::GetTime());

	ssErrorStream << "\n";
	ssErrorStream << std::format("This: {} ({:#x})\n", ResolveAddress(uintptr_t(s_lpParam)), uintptr_t(s_lpParam));
	ssErrorStream << std::format("RIP: {} ({:#x})\n", ResolveAddress(ExceptionInfo->ContextRecord->Rip), ExceptionInfo->ContextRecord->Rip);
	ssErrorStream << std::format("RAX: {:#x}\n", ExceptionInfo->ContextRecord->Rax);
	ssErrorStream << std::format("RCX: {:#x}\n", ExceptionInfo->ContextRecord->Rcx);
	ssErrorStream << std::format("RDX: {:#x}\n", ExceptionInfo->ContextRecord->Rdx);
	ssErrorStream << std::format("RBX: {:#x}\n", ExceptionInfo->ContextRecord->Rbx);
	ssErrorStream << std::format("RSP: {:#x}\n", ExceptionInfo->ContextRecord->Rsp);
	ssErrorStream << std::format("RBP: {:#x}\n", ExceptionInfo->ContextRecord->Rbp);
	ssErrorStream << std::format("RSI: {:#x}\n", ExceptionInfo->ContextRecord->Rsi);
	ssErrorStream << std::format("RDI: {:#x}\n", ExceptionInfo->ContextRecord->Rdi);

	ssErrorStream << "\n";
	if (auto vTrace = StackTrace(ExceptionInfo->ContextRecord);
		!vTrace.empty())
	{
		for (int i = 0; i < vTrace.size(); i++)
		{
			Frame_t& tFrame = vTrace[i];

			ssErrorStream << std::format("{}: ", i + 1);
			if (tFrame.m_uBase)
				ssErrorStream << std::format("{}+{:#x}", tFrame.m_sModule, tFrame.m_uAddress - tFrame.m_uBase);
			else
				ssErrorStream << ResolveAddress(tFrame.m_uAddress);
			if (!tFrame.m_sFile.empty())
				ssErrorStream << std::format(" ({} L{})", tFrame.m_sFile, tFrame.m_uLine);
			if (!tFrame.m_sName.empty())
				ssErrorStream << std::format(" ({})", tFrame.m_sName);
			ssErrorStream << "\n";
		}
	}
	else
	{
		ssErrorStream << ResolveAddress(uintptr_t(ExceptionInfo->ExceptionRecord->ExceptionAddress));
		ssErrorStream << "\n";
	}

	const std::string sLog = ssErrorStream.str();

	std::vector<std::string> vPaths;
	vPaths.emplace_back(F::Configs.m_sConfigPath + "crash_log.txt");
	char sTemp[MAX_PATH] = {};
	if (GetTempPathA(MAX_PATH, sTemp))
		vPaths.emplace_back(std::string(sTemp) + "Phobia\\crash_log.txt");

	for (const auto& sPath : vPaths)
	{
		try
		{
			std::ofstream file;
			file.open(sPath, std::ios_base::app);
			file << sLog + "\n\n\n";
			file.close();
		}
		catch (...) {}
	}

	bool bSent = false;
	if (Vars::Debug::SendCrashLogs.Value)
	{
		static std::atomic_bool bUploading = false;
		if (!bUploading.exchange(true))
		{
			bSent = UploadCrashLog(sLog);
			bUploading = false;
		}
	}

	ssErrorStream << "\n";
	ssErrorStream << "Ctrl + C to copy. \n";
	ssErrorStream << "Logged to Phobia\\crash_log.txt. ";
	if (Vars::Debug::SendCrashLogs.Value)
		ssErrorStream << (bSent ? "Crash log sent. " : "Crash log upload failed. ");

	switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
	case STATUS_ACCESS_VIOLATION:
	case STATUS_STACK_OVERFLOW:
	case STATUS_HEAP_CORRUPTION:
		SDK::Output("Unhandled exception", ssErrorStream.str().c_str(), {}, OUTPUT_DEBUG, nullptr, MB_OK | MB_ICONERROR);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void CExceptionHandler::Initialize(LPVOID lpParam)
{
	s_lpParam = lpParam;
	s_uModuleBase = uintptr_t(lpParam);
	if (auto pDos = PIMAGE_DOS_HEADER(lpParam); pDos && pDos->e_magic == IMAGE_DOS_SIGNATURE)
	{
		if (auto pNt = PIMAGE_NT_HEADERS(uintptr_t(lpParam) + pDos->e_lfanew); pNt && pNt->Signature == IMAGE_NT_SIGNATURE)
			s_uModuleSize = pNt->OptionalHeader.SizeOfImage;
	}

	s_pHandle = AddVectoredExceptionHandler(1, ExceptionFilter);
}
void CExceptionHandler::Unload()
{
	RemoveVectoredExceptionHandler(s_pHandle);
}