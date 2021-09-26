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

	PCSX2Interface::PCSX2Interface(DisassemblyDialog* dis) : WinSockInterface(Architecture::MIPSR5900) {
		m_disDialog = dis;
		m_initialized = false;
	}

	PCSX2Interface::~PCSX2Interface() {
	}

	void PCSX2Interface::Init() {
		if (m_initialized) return;

		/*
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

			// for some reason, GDB doesn't respect the actual register sizes and
			// will actually reject the target description if they are described
			// accurately
			// if (bits == 128) bits = 32;

			for (u8 r = 0; r < rc; r++) {
				GDB::RegisterType tp = alltypes[ctypes[c]];
				for (u8 s = 0;s < scount;s++) {
					if (categories[c] == special[s].cat && r == special[s].num) {
						tp = special[s].tp;
						break;
					}
				}
				
				if (tp == RegisterType::GeneralPurpose && r == 33) {
					// R59000 apparently doesn't have the sr register, bug GDB expects
					// it to be right before the lo register

					RegisterID id = DefineRegister("sr", 64, tp);
					m_regInfo[id] = { -1, -1, 64 };
				}

				// hack to make GDB happy...
				if (tp == RegisterType::GeneralPurpose) {
					bits = 64;
					if (r == 33 || r == 34) bits = 32; // 33 = lo, 34 = hi
					else if (r == 32) {
						// 32 = pc
						// GDB expects the sequence to be: ..., ra, sr, lo, hi, bad, cause, pc
						// pc must be defined later

						// fuggit, I'll just do this manually...
						continue;
					}
				} else if (tp == RegisterType::FloatingPoint) bits = 32;

				RegisterID id = DefineRegister(r5900Debug.getRegisterName(categories[c], r), bits, tp);
				m_regInfo[id] = { categories[c], r, bits };
			}
		}
		*/

		for (u8 r = 0;r < 32;r++) {
			RegisterID id = DefineRegister(r5900Debug.getRegisterName(EECAT_GPR, r), 64, RegisterType::GeneralPurpose);
			m_regInfo[id] = { EECAT_GPR, r, 64 };
		}

		m_regInfo[DefineRegister("sr"   , 64, RegisterType::GeneralPurpose)] = { -1, -1, 64 };
		m_regInfo[DefineRegister("lo"   , 64, RegisterType::GeneralPurpose)] = { EECAT_GPR, 34, 64 };
		m_regInfo[DefineRegister("hi"   , 64, RegisterType::GeneralPurpose)] = { EECAT_GPR, 33, 64 };
		m_regInfo[DefineRegister("bad"  , 64, RegisterType::GeneralPurpose)] = { -1, -1, 64 };
		m_regInfo[DefineRegister("cause", 64, RegisterType::GeneralPurpose)] = { -1, -1, 64 };
		m_regInfo[DefineRegister("pc"   , 64, RegisterType::GeneralPurpose)] = { EECAT_GPR, 32, 64 };

		for (u8 r = 0;r < 32;r++) {
			RegisterID id = DefineRegister(r5900Debug.getRegisterName(EECAT_FPR, r), 32, RegisterType::FloatingPoint);
			m_regInfo[id] = { EECAT_FPR, r, 32 };
		}

		m_initialized = true;
	}

	void PCSX2Interface::EnableInSeparateThread() {
		if (!m_initialized) Init();
		if (m_gdbThread.joinable()) m_gdbThread.join();
		m_gdbThread = std::thread(GDBThread, this);
	}

	int PCSX2Interface::DebugPrint(const char* msg) {
		Console.WriteLn("[GDB] %s", msg);
		return strlen(msg);
	}

	void PCSX2Interface::PacketReceived(const char* pkt) {
		DebugPrintf("Received: %s", pkt);
	}

	ProcessStatus PCSX2Interface::Status() {
		if (r5900Debug.isCpuPaused()) return ProcessStatus::Stopped;
		return ProcessStatus::Running;
	}

	void PCSX2Interface::GDB_Connected() {
		DebugPrint("GDB client connected");
		if (!r5900Debug.isAlive()) return;

		m_disDialog->pauseExecution();

		if (!r5900Debug.isCpuPaused()) {
			DebugPrint("Failed to stop execution for GDB");
		}
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
		u128 curPc = r5900Debug.getRegister(EECAT_GPR, 32);
		m_disDialog->stepInto();
		// wait for cpu to reach the next instruction
		// todo: timeout after x seconds
		while(true) {
			if (!r5900Debug.isAlive() || curPc != r5900Debug.getRegister(EECAT_GPR, 32)) {
				break;
			}
		}
		return Result::Success;
	}

	Result PCSX2Interface::ContinueExecution() {
		if (!r5900Debug.isAlive()) return Result::InternalError;
		m_disDialog->resumeExecution();

		// wait for cpu resume
		// todo: timeout after x seconds
		while(true) {
			if (!r5900Debug.isAlive() || !r5900Debug.isCpuPaused()) {
				break;
			}
		}

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
		const u8 bytes = info.bits / 8;

		DebugPrintf("Fetching register %s (%d B)", RegisterName(reg), bytes);
		const u128 val = r5900Debug.getRegister(info.cat, info.num);
		for (u8 b = 0;b < bytes;b++) ((u8*)dest)[b] = val._u8[(bytes - 1) - b];

		return Result::Success;
	}

	Result PCSX2Interface::WriteRegister(RegisterID reg, const void* src) {
		const auto& info = m_regInfo[reg];
		const u8 bytes = info.bits / 8;

		u128 val;
		val.lo = val.hi = 0;
		for (u8 b = 0;b < bytes;b++) val._u8[(bytes - 1) - b] = ((u8*)src)[b];

		r5900Debug.setRegister(info.cat, info.num, val);
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