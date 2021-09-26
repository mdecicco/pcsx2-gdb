#pragma once
#include "cpp_gdb.h"
#include <string.h>

namespace GDB {
    class WinSockInterface : public Interface {
        public:
            WinSockInterface(Architecture arch);
            ~WinSockInterface();

            virtual bool IO_Open(unsigned short port);
			virtual void GDB_Connected();
            virtual void IO_Close();
            virtual size_t IO_Peek();
            virtual Result IO_Read(void* dest, size_t size, size_t* bytesRead);
            virtual Result IO_Write(const void* src, size_t size);
            virtual Result IO_Poll();

            bool IsListening() const;
            void StopListening();

        protected:
            bool m_listening;
            unsigned int m_socket;
            unsigned int m_conn;
            void* m_server;
            void* m_client;
    };
};

