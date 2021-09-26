#pragma once
#include <mutex>

namespace GDB {
    enum class ProcessStatus {
        Invalid,
        Running,
        Stopped
    };

    enum class Result {
        Success,
        InvalidParameter,
        NoMemory,
        TryAgain,
        InternalError,
        PeerDisconnected,
        NotSupported,
        ProtocolViolation,
        BufferOverflow,
        NotFound
    };

    enum class Architecture {
        ARM,
        X86,
        AMD64,
        MIPS,
        MIPSR5900
    };

    enum class TracepointType {
        Invalid,
        ExecutionSoftware,
        ExecutionHardware,
        MemoryRead,
        MemoryWrite,
        MemoryAccess
    };

    enum class TracepointAction {
        Stop
    };

    enum class RegisterType {
        GeneralPurpose,
        FloatingPoint,
        ProgramCounter,
        StackPointer,
        CodePointer,
        Status
    };

    class Interface {
        public:
            typedef Result (*CommandCallback)(Interface* /* this */, const char* /* args */);
            typedef int RegisterID;

            Interface(Architecture arch);
            ~Interface();

            // Start/Stop GDB server
            void Enable(unsigned short port);
            void Disable();
			bool IsEnabled() const;
			void Lock();
			void Unlock();

            // Custom GDB command helpers
            void DefineCustomCommand(const char* cmd, CommandCallback cb, const char* desc = nullptr);
            CommandCallback GetCommandCallback(const char* cmd);

            // CPU register helpers
            RegisterID DefineRegister(const char* name, unsigned char bits, RegisterType type);
            int RegisterCount() const;
            const char* RegisterName(RegisterID reg) const;
            int RegisterBits(RegisterID reg) const;
            Result Run();

            // IO methods
            virtual bool IO_Open(unsigned short port);
            virtual void IO_Close();
            virtual size_t IO_Peek();
            virtual Result IO_Read(void* dest, size_t size, size_t* bytesRead);
            virtual Result IO_Write(const void* src, size_t size);
            virtual Result IO_Poll();

            // Misc.
            int DebugPrintf(const char* fmt, ...);

            // Implementation specific methods
            virtual int DebugPrint(const char* str);
            virtual void* Allocate(size_t size);
            virtual void Free(void* mem);
            virtual ProcessStatus Status();
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
            virtual void PacketReceived(const char* pkt);

        protected:
            Architecture m_arch;
            int m_socket;
            void* m_ctx;
            void* m_io;
            void* m_if;
            void* m_registers;
            int m_registerCount;
            int m_registerCapacity;
            void* m_customCommands;
            CommandCallback* m_customCommandCallbacks;
            int m_customCommandCount;
            int m_customCommandCapacity;
            bool m_enabled;
            std::mutex m_mutex;
    };
};

