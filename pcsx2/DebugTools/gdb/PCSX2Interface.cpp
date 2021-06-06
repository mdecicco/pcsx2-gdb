#include "PrecompiledHeader.h"
#include "PCSX2Interface.h"
#include <R5900.h>
#include "DebugTools/DebugInterface.h"
#include "Debugger/DisassemblyDialog.h"
#include "AppCoreThread.h"
#include "App.h"

extern "C" {
	#include "gdb.h"
}


// todo:
// - Disable GDB server when DisassemblyDialog closes
// - Figure out how to implement the calling of the GDB run loop
// - etc

#define _ctx ((GDBSTUBCTX)m_ctx)

namespace GDB {
	void GDBThread(PCSX2Interface* gdb) {
		gdb->Enable(6169); // todo: Make port configurable
		if (gdb->IsEnabled()) gdb->Run();
	}

	PCSX2Interface::PCSX2Interface(DisassemblyDialog* dis) : WinSockInterface(Architecture::MIPS) {
		m_disDialog = dis;

		GDB::RegisterType alltypes[] = {
			RegisterType::GeneralPurpose,
			RegisterType::FloatingPoint,
			RegisterType::CodePointer,
			RegisterType::ProgramCounter,
			RegisterType::StackPointer,
			RegisterType::Status
		};

		int ccount = 2;
		int categories[] = { EECAT_GPR, EECAT_FPR }; //{ EECAT_GPR, EECAT_CP0, EECAT_FPR, EECAT_FCR, EECAT_VU0F, EECAT_VU0I, EECAT_GSPRIV, EECAT_COUNT };
		u8  ctypes    [] = { 0        , 1         }; //{ 0        , 0        , 0        , 0        , 0         , 0         , 0           , 0           };

		struct sp { int cat; int num; GDB::RegisterType tp; };
		int scount = 3;
		sp special[] = {
			{ EECAT_GPR, 29, RegisterType::StackPointer   }, // sp
			{ EECAT_GPR, 31, RegisterType::CodePointer    }, // ra
			{ EECAT_GPR, 32, RegisterType::ProgramCounter }  // pc
		};

		for (u8 c = 0;c < ccount;c++) {
			int rc = r5900Debug.getRegisterCount(c);
			int bits = r5900Debug.getRegisterSize(c);

			for (u8 r = 0; r < rc; r++) {
				GDB::RegisterType tp = alltypes[ctypes[c]];
				for (u8 s = 0;s < scount;s++) {
					if (categories[c] == special[s].cat && r == special[s].num) {
						tp = special[s].tp;
						break;
					}
				}

				RegisterID id = DefineRegister(r5900Debug.getRegisterName(categories[c], r), bits, tp);
				m_regInfo[id] = std::pair<int, int>(categories[c], r);
			}
		}
	}

	PCSX2Interface::~PCSX2Interface() {
	}

	void PCSX2Interface::EnableInSeparateThread() {
		if (m_gdbThread.joinable()) m_gdbThread.join();
		m_gdbThread = std::thread(GDBThread, this);
	}

	int PCSX2Interface::DebugPrint(const char* msg) {
		Console.WriteLn(msg);
		return strlen(msg);
	}

	void PCSX2Interface::PacketReceived(const char* pkt) {
		DebugPrintf("Packet: %s", pkt);
	}

	Result PCSX2Interface::StopExecution() {
		if (!r5900Debug.isAlive()) return Result::TryAgain;
		m_disDialog->pauseExecution();
		if (!r5900Debug.isCpuPaused()) return Result::InternalError;
		return Result::Success;
	}

	Result PCSX2Interface::RestartExecution() {
		SysCoreThread& core = GetCoreThread();
		core.Reset();
		return Result::Success;
	}

	Result PCSX2Interface::KillExecution() {
		UI_DisableSysShutdown();
		Console.SetTitle("PCSX2 Program Log");
		CoreThread.Reset();
		return Result::Success;
	}

	Result PCSX2Interface::SingleStepExecution() {
		m_disDialog->stepInto();
		return Result::Success;
	}

	Result PCSX2Interface::ContinueExecution() {
		if (!r5900Debug.isAlive()) return Result::TryAgain;
		m_disDialog->resumeExecution();
		if (r5900Debug.isCpuPaused()) return Result::InternalError;
		return Result::Success;
	}

	Result PCSX2Interface::ReadMem(size_t address, size_t size, void* dest) {
		if (!r5900Debug.isValidAddress(address) || !dest) return Result::InvalidParameter;
		u8* dst = (u8*)dest;
		for (u32 i = 0;i < size;i++) {
			if (!r5900Debug.isValidAddress(address + i)) return Result::InvalidParameter;
			dst[i] = r5900Debug.read8(address + i);
		}
		return Result::Success;
	}

	Result PCSX2Interface::WriteMem(size_t address, size_t size, const void* src) {
		if (!r5900Debug.isValidAddress(address) || !src) return Result::InvalidParameter;
		for (u32 i = 0; i < size; i++) {
			if (!r5900Debug.isValidAddress(address + i)) return Result::InvalidParameter;
			r5900Debug.write8(address + i, ((u8*)src)[i]);
		}
		return Result::Success;
	}

	Result PCSX2Interface::ReadRegister(RegisterID reg, void* dest) {
		const auto& info = m_regInfo[reg];
		const u8 bytes = RegisterBits(reg) / 8;

		const u128 val = r5900Debug.getRegister(info.first, info.second);
		for (u8 b = 0;b < bytes;b++) ((u8*)dest)[b] = val._u8[b];

		return Result::Success;
	}

	Result PCSX2Interface::WriteRegister(RegisterID reg, const void* src) {
		const auto& info = m_regInfo[reg];
		const u8 bytes = RegisterBits(reg) / 8;

		u128 val;
		for (u8 b = 0;b < bytes;b++) val._u8[b] = ((u8*)src)[b];

		r5900Debug.setRegister(info.first, info.second, val);
		return Result::Success;
	}

	Result PCSX2Interface::CreateTracepoint(size_t address, TracepointType type, TracepointAction action) {
		switch (type) {
			case TracepointType::ExecutionHardware:
			case TracepointType::ExecutionSoftware: {
				CBreakPoints::AddBreakPoint(m_disDialog->currentCpu->getCpu()->getCpuType(), address);
				break;
			}
			case TracepointType::MemoryRead: {
				CBreakPoints::AddMemCheck(m_disDialog->currentCpu->getCpu()->getCpuType(), address, address + 1, MemCheckCondition::MEMCHECK_READ, MemCheckResult::MEMCHECK_BREAK);
				break;
			}
			case TracepointType::MemoryWrite: {
				CBreakPoints::AddMemCheck(m_disDialog->currentCpu->getCpu()->getCpuType(), address, address + 1, MemCheckCondition::MEMCHECK_WRITE, MemCheckResult::MEMCHECK_BREAK);
				break;
			}
			case TracepointType::MemoryAccess: {
				CBreakPoints::AddMemCheck(m_disDialog->currentCpu->getCpu()->getCpuType(), address, address + 1, MemCheckCondition::MEMCHECK_READWRITE, MemCheckResult::MEMCHECK_BREAK);
				break;
			}
		}
		return Result::Success;
	}

	Result PCSX2Interface::ClearTracepoint(size_t address){
		CBreakPoints::RemoveBreakPoint(m_disDialog->currentCpu->getCpu()->getCpuType(), address);
		return Result::Success;
	}

	Result PCSX2Interface::InvalidCommand(const char* cmd) {
		return Result::NotSupported;
	}
};