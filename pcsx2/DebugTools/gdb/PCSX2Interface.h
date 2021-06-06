#pragma once
#include "WinSockInterface.h"
#include <unordered_map>

class DisassemblyDialog;

namespace GDB {
    class PCSX2Interface : public WinSockInterface {
        public:
            PCSX2Interface(DisassemblyDialog* dis);
			~PCSX2Interface();

			virtual int DebugPrint(const char* msg);
			virtual void PacketReceived(const char* pkt);
			virtual Result StopExecution();
			virtual Result RestartExecution();
			virtual Result KillExecution();
			virtual Result SingleStepExecution();
			virtual Result ContinueExecution();
			virtual Result ReadMem(size_t address, size_t size, void* dest);
			virtual Result WriteMem(size_t address, size_t size, const void* src);
			virtual Result ReadRegister(RegisterID reg, void* dest);
			virtual Result WriteRegister(RegisterID reg, const void* src);
			virtual Result CreateTracepoint(size_t address, TracepointType type, TracepointAction action);
			virtual Result ClearTracepoint(size_t address);
			virtual Result InvalidCommand(const char* cmd);

		private:
			DisassemblyDialog* m_disDialog;
			std::unordered_map<RegisterID, std::pair<int, int>> m_regInfo; // RegisterID -> (category, number)
    };
}
