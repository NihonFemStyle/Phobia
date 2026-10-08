#pragma once

#include "../../Utils/Macros/Macros.h"

#include <windows.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class CDiscordRPC
{
private:
	struct Snapshot_t
	{
		bool m_bActive = false;
		bool m_bInParty = false;
		int m_iPartySize = 1;
		uint64_t m_uStartTime = 0;
		std::string m_sDetails;
		std::string m_sState;
		std::string m_sSmallImage;
		std::string m_sSmallText;
		std::string m_sPartyID;
	};

	enum DiscordOpcode
	{
		Opcode_Handshake = 0,
		Opcode_Frame = 1,
		Opcode_Heartbeat = 2,
		Opcode_Subscribe = 3,
		Opcode_Unsubscribe = 4,
		Opcode_Close = 5
	};

public:
	void Start();
	void Unload();
	void Run();

private:
	void WorkerMain();

	bool Connect();
	bool Handshake();

	void SendFrame(uint32_t uOpcode, const std::string& sPayload);
	bool ReadFrame(std::string& sOut);
	void SendActivity();
	void ClearActivity();
	void CloseConnection();

	std::string BuildActivityJSON(const Snapshot_t& tSnapshot);
	std::string GetMode();
	std::string FormatMap(const char* sLevel);

	std::thread m_Worker;
	std::atomic<bool> m_bRunning = false;
	std::atomic<bool> m_bUnload = false;
	std::atomic<bool> m_bEnabled = false;
	std::atomic<bool> m_bDirty = false;
	uint64_t m_iNonce = 0;

	Snapshot_t m_Snapshot = {};
	std::mutex m_SnapshotMutex;

	HANDLE m_hPipe = INVALID_HANDLE_VALUE;

	std::string m_sCurrentMap = "";
	uint64_t m_uStartTime = 0;
};

ADD_FEATURE(CDiscordRPC, DiscordRPC);