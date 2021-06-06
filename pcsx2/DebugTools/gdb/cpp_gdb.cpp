#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "cpp_gdb.h"

extern "C" {
    #include "gdb.h"
}

#define _ctx ((GDBSTUBCTX)m_ctx)
#define _io ((GDBSTUBIOIF*)m_io)
#define _if ((GDBSTUBIF*)m_if)
#define _regs ((GDBSTUBREG*)m_registers)
#define _cmds ((GDBSTUBCMD*)m_customCommands)

namespace GDB {


    // Enum conversion helpers


    GDBSTUBREGTYPE RegTp(RegisterType tp) {
        switch (tp) {
            case RegisterType::GeneralPurpose: return GDBSTUBREGTYPE_GP;
            case RegisterType::FloatingPoint: return GDBSTUBREGTYPE_FPR;
            case RegisterType::ProgramCounter: return GDBSTUBREGTYPE_PC;
            case RegisterType::StackPointer: return GDBSTUBREGTYPE_STACK_PTR;
            case RegisterType::CodePointer: return GDBSTUBREGTYPE_CODE_PTR;
            case RegisterType::Status: return GDBSTUBREGTYPE_STATUS;
        }
        return GDBSTUBREGTYPE_INVALID;
    }

    GDBSTUBTGTARCH Arch(Architecture arch) {
        switch (arch) {
            case Architecture::ARM: return GDBSTUBTGTARCH_ARM;
            case Architecture::X86: return GDBSTUBTGTARCH_X86;
            case Architecture::AMD64: return GDBSTUBTGTARCH_AMD64;
            case Architecture::MIPS: return GDBSTUBTGTARCH_MIPS;
        }
        return GDBSTUBTGTARCH_INVALID;
    }

    GDBSTUBTGTSTATE State(ProcessStatus status) {
        switch (status) {
            case ProcessStatus::Running: return GDBSTUBTGTSTATE_RUNNING;
            case ProcessStatus::Stopped: return GDBSTUBTGTSTATE_STOPPED;
            case ProcessStatus::Invalid: return GDBSTUBTGTSTATE_INVALID;
        }
        return GDBSTUBTGTSTATE_INVALID;
    }

    TracepointType tpType(GDBSTUBTPTYPE tp) {
        switch(tp) {
            case GDBSTUBTPTYPE::GDBSTUBTPTYPE_EXEC_SW: return TracepointType::ExecutionSoftware;
            case GDBSTUBTPTYPE::GDBSTUBTPTYPE_EXEC_HW: return TracepointType::ExecutionHardware;
            case GDBSTUBTPTYPE::GDBSTUBTPTYPE_MEM_READ: return TracepointType::MemoryRead;
            case GDBSTUBTPTYPE::GDBSTUBTPTYPE_MEM_WRITE: return TracepointType::MemoryWrite;
            case GDBSTUBTPTYPE::GDBSTUBTPTYPE_MEM_ACCESS: return TracepointType::MemoryAccess;
            default: return TracepointType::Invalid;
        }

        return TracepointType::Invalid;
    }
    
    TracepointAction tpAction(GDBSTUBTPACTION a) {
        switch(a) {
            case GDBSTUBTPACTION::GDBSTUBTPACTION_STOP:
            default: return TracepointAction::Stop;
        }

        return TracepointAction::Stop;
    }

    int cmdStatus(Result result) {
        switch(result) {
            case Result::Success: return 0;
            case Result::InvalidParameter: return GDBSTUB_ERR_INVALID_PARAMETER;
            case Result::NoMemory: return GDBSTUB_ERR_NO_MEMORY;
            case Result::TryAgain: return GDBSTUB_INF_TRY_AGAIN;
            case Result::InternalError: return GDBSTUB_ERR_INTERNAL_ERROR;
            case Result::PeerDisconnected: return GDBSTUB_ERR_PEER_DISCONNECTED;
            case Result::NotSupported: return GDBSTUB_ERR_NOT_SUPPORTED;
            case Result::ProtocolViolation: return GDBSTUB_ERR_PROTOCOL_VIOLATION;
            case Result::BufferOverflow: return GDBSTUB_ERR_BUFFER_OVERFLOW;
            case Result::NotFound: return GDBSTUB_ERR_NOT_FOUND;
        }

        return GDBSTUB_ERR_INTERNAL_ERROR;
    }

    Result cmdResult(int result) {
        switch(result) {
            case GDBSTUB_INF_SUCCESS: return Result::Success;
            case GDBSTUB_ERR_INVALID_PARAMETER: return Result::InvalidParameter;
            case GDBSTUB_ERR_NO_MEMORY: return Result::NoMemory;
            case GDBSTUB_INF_TRY_AGAIN: return Result::TryAgain;
            case GDBSTUB_ERR_INTERNAL_ERROR: return Result::InternalError;
            case GDBSTUB_ERR_PEER_DISCONNECTED: return Result::PeerDisconnected;
            case GDBSTUB_ERR_NOT_SUPPORTED: return Result::NotSupported;
            case GDBSTUB_ERR_PROTOCOL_VIOLATION: return Result::ProtocolViolation;
            case GDBSTUB_ERR_BUFFER_OVERFLOW: return Result::BufferOverflow;
            case GDBSTUB_ERR_NOT_FOUND: return Result::NotFound;
        }

        return Result::InternalError;
    }


    // Stub functions


    int customCommand(GDBSTUBCTX ctx, PCGDBSTUBOUTHLP hlp, void* pCmd, const char* args, void* data) {
        Interface* i = (Interface*)data;
        Interface::CommandCallback cb = i->GetCommandCallback(((GDBSTUBCMD*)pCmd)->pszCmd);
        
        if (cb) {
            return cmdStatus(cb(i, args));
        }
        
        i->DebugPrint("InternalError: Command callback not found");
        return GDBSTUB_ERR_INTERNAL_ERROR;
    }

    void *gdbStubIfMemAlloc(GDBSTUBCTX hGdbStubCtx, void *pvUser, size_t cb) {
        Interface* i = (Interface*)pvUser;
        return i->Allocate(cb);
    }

    void gdbStubIfMemFree(GDBSTUBCTX hGdbStubCtx, void *pvUser, void *pv) {
        Interface* i = (Interface*)pvUser;
        i->Free(pv);
    }

    GDBSTUBTGTSTATE gdbStubIfTgtGetState(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return State(i->Status());
    }

    int gdbStubIfTgtStop(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->StopExecution());
    }

    int gdbStubIfTgtRestart(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->RestartExecution());
    }

    int gdbStubIfTgtKill(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->KillExecution());
    }

    int gdbStubIfTgtStep(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->SingleStepExecution());
    }

    int gdbStubIfTgtCont(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->ContinueExecution());
    }

    int gdbStubIfTgtMemRead(GDBSTUBCTX hGdbStubCtx, void *pvUser, GDBTGTMEMADDR GdbTgtMemAddr, void *pvDst, size_t cbRead) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->ReadMem(GdbTgtMemAddr, cbRead, pvDst));
    }

    int gdbStubIfTgtMemWrite(GDBSTUBCTX hGdbStubCtx, void *pvUser, GDBTGTMEMADDR GdbTgtMemAddr, const void *pvSrc, size_t cbWrite) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->WriteMem(GdbTgtMemAddr, cbWrite, pvSrc));
    }

    int gdbStubIfTgtRegsRead(GDBSTUBCTX hGdbStubCtx, void *pvUser, uint32_t *paRegs, uint32_t cRegs, void *pvDst) {
        Interface* i = (Interface*)pvUser;

        uint8_t* dest = (uint8_t*)pvDst;

        for (uint32_t r = 0; r < cRegs; r++) {
            int bytes = i->RegisterBits((Interface::RegisterID)paRegs[r]) / 8;
            Result result = i->ReadRegister((Interface::RegisterID)paRegs[r], (void*)dest);
            if (result != Result::Success) return cmdStatus(result);
            dest += bytes;
        }

        return GDBSTUB_INF_SUCCESS;
    }

    int gdbStubIfTgtRegsWrite(GDBSTUBCTX hGdbStubCtx, void *pvUser, uint32_t *paRegs, uint32_t cRegs, const void *pvSrc) {
        Interface* i = (Interface*)pvUser;
        
        // After inspecting gdb-stub.c, cRegs will always be 1
        return cmdStatus(i->WriteRegister((Interface::RegisterID)*paRegs, pvSrc));
    }

    int gdbStubIfTgtTpSet(GDBSTUBCTX hGdbStubCtx, void *pvUser, GDBTGTMEMADDR GdbTgtTpAddr, GDBSTUBTPTYPE enmTpType, GDBSTUBTPACTION enmTpAction) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->CreateTracepoint(GdbTgtTpAddr, tpType(enmTpType), tpAction(enmTpAction)));
    }

    int gdbStubIfTgtTpClear(GDBSTUBCTX hGdbStubCtx, void *pvUser, GDBTGTMEMADDR GdbTgtTpAddr) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->ClearTracepoint(GdbTgtTpAddr));
    }

    int gdbStubIfMonCmd(GDBSTUBCTX hGdbStubCtx, PCGDBSTUBOUTHLP pHlp, const char *pszCmd, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->InvalidCommand(pszCmd));
    }
    
    void gdbStubIfPrePkt(GDBSTUBCTX hGdbStubCtx, const char* pkt, uint16_t sz, void* pvUser) {
        Interface* i = (Interface*)pvUser;
        static char buf[512] = { 0 };
        for (int i = 0;i < sz;i++) buf[i] = pkt[i];
        buf[sz] = 0;
        i->PacketReceived(buf);
	}

	void gdbStubIfLock(void* pvUser) {
		Interface* i = (Interface*)pvUser;
		i->Lock();
	}

	void gdbStubIfUnlock(void* pvUser) {
		Interface* i = (Interface*)pvUser;
		i->Unlock();
	}

    
    // IO functions


    size_t gdbStubIoIfPeek(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return i->IO_Peek();
    }

    int gdbStubIoIfRead(GDBSTUBCTX hGdbStubCtx, void *pvUser, void *pvDst, size_t cbRead, size_t *pcbRead) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->IO_Read(pvDst, cbRead, pcbRead));
    }

    int gdbStubIoIfWrite(GDBSTUBCTX hGdbStubCtx, void *pvUser, const void *pvPkt, size_t cbPkt) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->IO_Write(pvPkt, cbPkt));
    }

    int gdbStubIoIfPoll(GDBSTUBCTX hGdbStubCtx, void *pvUser) {
        Interface* i = (Interface*)pvUser;
        return cmdStatus(i->IO_Poll());
    }


    // Interface methods


    Interface::Interface(Architecture arch) {
        m_enabled = false;
        m_ctx = nullptr;
        m_io = nullptr;
        m_if = nullptr;
        m_registers = nullptr;
        m_customCommands = nullptr;
        m_customCommandCallbacks = nullptr;
        m_socket = 0;
        m_arch = arch;
        m_registerCapacity = 32;
        m_registerCount = 0;
        m_customCommandCapacity = 8;
        m_customCommandCount = 0;

        m_io = malloc(sizeof(GDBSTUBIOIF));
        m_if = malloc(sizeof(GDBSTUBIF));
        m_registers = malloc(sizeof(GDBSTUBREG) * m_registerCapacity);
        m_customCommands = malloc(sizeof(GDBSTUBCMD) * m_customCommandCapacity);
        m_customCommandCallbacks = (CommandCallback*)malloc(sizeof(CommandCallback) * m_customCommandCapacity);

        if (!m_io) { DebugPrint("InternalError: Failed to allocate space for IO interface."); return; }
        if (!m_if) { DebugPrint("InternalError: Failed to allocate space for GDB interface."); return; }
        if (!m_registers) { DebugPrint("InternalError: Failed to allocate space for register info."); return; }
        if (!m_customCommands) { DebugPrint("InternalError: Failed to allocate space for custom callback info."); return; }
        if (!m_customCommandCallbacks) { DebugPrint("InternalError: Failed to allocate space for custom callback function pointers."); return; }


        _if->enmArch = Arch(m_arch);
        _if->pfnMemAlloc = gdbStubIfMemAlloc;
        _if->pfnMemFree = gdbStubIfMemFree;
        _if->pfnTgtGetState = gdbStubIfTgtGetState;
        _if->pfnTgtStop = gdbStubIfTgtStop;
        _if->pfnTgtRestart = gdbStubIfTgtRestart;
        _if->pfnTgtKill = gdbStubIfTgtKill;
        _if->pfnTgtStep = gdbStubIfTgtStep;
        _if->pfnTgtCont = gdbStubIfTgtCont;
        _if->pfnTgtMemRead = gdbStubIfTgtMemRead;
        _if->pfnTgtMemWrite = gdbStubIfTgtMemWrite;
        _if->pfnTgtRegsRead = gdbStubIfTgtRegsRead;
        _if->pfnTgtRegsWrite = gdbStubIfTgtRegsWrite;
        _if->pfnTgtTpSet = gdbStubIfTgtTpSet;
        _if->pfnTgtTpClear = gdbStubIfTgtTpClear;
        _if->pfnMonCmd = gdbStubIfMonCmd;
        _if->pfnPktCb = gdbStubIfPrePkt;
        _if->pfnLock = gdbStubIfLock;
        _if->pfnUnlock = gdbStubIfUnlock;

        _io->pfnPeek = gdbStubIoIfPeek;
        _io->pfnRead = gdbStubIoIfRead;
        _io->pfnWrite = gdbStubIoIfWrite;
        _io->pfnPoll = gdbStubIoIfPoll;

        _cmds[0] = { nullptr, nullptr, nullptr };
        _regs[0] = { nullptr, 0, GDBSTUBREGTYPE_INVALID };
    }

    Interface::~Interface() {
        if (m_ctx) {
            if (m_enabled) GDBStubCtxDestroy(_ctx);
            m_ctx = nullptr;
        }

        if (m_io) {
            free(m_io);
            m_io = nullptr;
        }

        if (m_if) {
            free(m_if);
            m_if = nullptr;
        }

        if (m_registers) {
            free(m_registers);
            m_registers = nullptr;
        }

        if (m_customCommands) {
            free(m_customCommands);
            m_customCommands = nullptr;

            free(m_customCommandCallbacks);
            m_customCommandCallbacks = nullptr;
        }

        m_registerCapacity = m_registerCount = 0;
        m_customCommandCapacity = m_customCommandCount = 0;
    }

    void Interface::Enable(unsigned short port) {
        if (m_enabled) return;

        _if->paRegs = _regs;
        _if->paCmds = _cmds;

        if (GDBStubCtxCreate((GDBSTUBCTX*)&m_ctx, _io, _if, (void*)this) != GDBSTUB_INF_SUCCESS) DebugPrint("Failed to create GDB stub context");
        else {
            m_enabled = IO_Open(port);
			if (!m_enabled) GDBStubCtxDestroy(_ctx);
        }
    }

    void Interface::Disable() {
        if (!m_enabled) return;
        m_mutex.lock();
        _ctx->doShutdown = true;
        m_mutex.unlock();

        while (!_ctx->didShutdown) {
            
        }

        GDBStubCtxDestroy(_ctx);
        IO_Close();
        m_enabled = false;
    }

    bool Interface::IsEnabled() const {
        return m_enabled;
	}
	
    void Interface::Lock() {
        m_mutex.lock();
    }

	void Interface::Unlock() {
        m_mutex.unlock();
    }

    void Interface::DefineCustomCommand(const char* cmd, Interface::CommandCallback cb, const char* desc) {
        if (m_enabled) {
            DebugPrint("InternalError: Cannot define custom commands once the GDB interface is started.");
            return;
        }

        if (m_customCommandCount == m_customCommandCapacity - 1) {
            size_t oldSize = sizeof(GDBSTUBCMD) * m_customCommandCapacity;
            size_t oldCbSize = sizeof(CommandCallback) * m_customCommandCapacity;
            m_customCommandCapacity += 4;

            void* newcmds = malloc(sizeof(GDBSTUBCMD) * m_customCommandCapacity);
            if (!newcmds) { DebugPrint("InternalError: Failed to allocate space for custom callback info."); return; }
            memcpy(newcmds, m_customCommands, oldSize);
            free(m_customCommands);
            m_customCommands = newcmds;

            CommandCallback* newcbs = (CommandCallback*)malloc(sizeof(CommandCallback) * m_customCommandCapacity);
            if (!newcbs) { DebugPrint("InternalError: Failed to allocate space for custom callback function pointers."); return; }
            memcpy(newcbs, m_customCommandCallbacks, oldCbSize);
            free(m_customCommandCallbacks);
            m_customCommandCallbacks = newcbs;
        }

        m_customCommandCallbacks[m_customCommandCount] = cb;
        _cmds[m_customCommandCount] = { cmd, desc, customCommand };
        _cmds[m_customCommandCount + 1] = { nullptr, nullptr, nullptr };
        m_customCommandCount++;
    }

    Interface::CommandCallback Interface::GetCommandCallback(const char* cmd) {
        for (int i = 0;i < m_customCommandCount;i++) {
            if (_stricmp(_cmds[i].pszCmd, cmd) == 0) return m_customCommandCallbacks[i];
        }

        return nullptr;
    }

    Interface::RegisterID Interface::DefineRegister(const char* name, unsigned char bits, RegisterType type) {
        if (m_enabled) {
            DebugPrint("InternalError: Cannot define registers once the GDB interface is started.");
            return -1;
        }

        if (m_registerCount == m_registerCapacity - 1) {
            size_t oldSize = sizeof(GDBSTUBREG) * m_registerCapacity;
            m_registerCapacity += 8;
            void* newregs = malloc(sizeof(GDBSTUBREG) * m_registerCapacity);
            if (!newregs) { DebugPrint("InternalError: Failed to allocate space for register info."); return -1; }
            memcpy(newregs, m_registers, oldSize);
            free(m_registers);
            m_registers = newregs;
        }

        _regs[m_registerCount] = { name, bits, RegTp(type) };
        _regs[m_registerCount + 1] = { nullptr, 0, GDBSTUBREGTYPE_INVALID };
        return m_registerCount++;
    }

    int Interface::RegisterCount() const {
        return m_registerCount;
    }

    int Interface::RegisterBits(RegisterID reg) const {
        return _regs[reg].cRegBits;
    }

    Result Interface::Run() {
        return cmdResult(GDBStubCtxRun(_ctx));
    }

    const char* Interface::RegisterName(RegisterID reg) const {
        return _regs[reg].pszName;
    }

    bool Interface::IO_Open(unsigned short port) {
        return false;
    }

    void Interface::IO_Close() {
    }

    size_t Interface::IO_Peek() {
        return 0;
    }

    Result Interface::IO_Read(void* dest, size_t size, size_t* bytesRead) {
        return Result::InternalError;
    }
    
    Result Interface::IO_Write(const void* src, size_t size) {
        return Result::InternalError;
    }
    
    Result Interface::IO_Poll() {
        return Result::InternalError;
    }

    int Interface::DebugPrintf(const char* fmt, ...) {
        va_list l;
        va_start(l, fmt);
        char msg[2048];
        int c = vsnprintf(msg, 2047, fmt, l);
        *(msg + c) = 0;
        va_end(l);

        return DebugPrint(msg);
    }

    int Interface::DebugPrint(const char* str) {
        return printf("%s\n", str);
    }

    void* Interface::Allocate(size_t size) {
        return malloc(size);
    }

    void Interface::Free(void* mem) {
        free(mem);
    }

    ProcessStatus Interface::Status() {
        return ProcessStatus::Stopped;
    }

    Result Interface::StopExecution() {
        return Result::NotSupported;
    }

    Result Interface::RestartExecution() {
        return Result::NotSupported;
    }

    Result Interface::KillExecution() {
        return Result::NotSupported;
    }

    Result Interface::SingleStepExecution() {
        return Result::NotSupported;
    }

    Result Interface::ContinueExecution() {
        return Result::NotSupported;
    }

    Result Interface::ReadMem(size_t address, size_t size, void* dest) {
        return Result::NotSupported;
    }

    Result Interface::WriteMem(size_t address, size_t size, const void* src) {
        return Result::NotSupported;
    }

    Result Interface::ReadRegister(RegisterID reg, void* dest) {
        return Result::NotSupported;
    }

    Result Interface::WriteRegister(RegisterID reg, const void* src) {
        return Result::NotSupported;
    }

    Result Interface::CreateTracepoint(size_t address, TracepointType type, TracepointAction action) {
        return Result::NotSupported;
    }

    Result Interface::ClearTracepoint(size_t address) {
        return Result::NotSupported;
    }

    Result Interface::InvalidCommand(const char* cmd) {
        return Result::NotSupported;
    }

    void Interface::PacketReceived(const char* pkt) {
        // DebugPrintf("Packet: %s", pkt);
    }
};