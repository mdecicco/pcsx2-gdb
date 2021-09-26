#include "WinSockInterface.h"
#include <winsock2.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

#define _srv ((sockaddr_in*)m_server)
#define _cli ((sockaddr_in*)m_client)
namespace GDB {
    WinSockInterface::WinSockInterface(Architecture arch) : Interface(arch), m_server(nullptr), m_socket(0), m_conn(0), m_listening(false) {
    }

    WinSockInterface::~WinSockInterface() {
        if (m_server) free(m_server);
        m_server = nullptr;
    }

    bool WinSockInterface::IO_Open(unsigned short port) {
        if (m_server) {
			DebugPrint("Socket is already open.");
            return false;
        }

        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            DebugPrintf("Failed to startup WSA <%d>.", WSAGetLastError());
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			return false;
        }

		Lock();
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(m_socket == INVALID_SOCKET) {
            DebugPrintf("Failed to open socket <%d>.", WSAGetLastError());
            WSACleanup();
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
			return false;
        }

        DebugPrint("Socket created");

        m_server = malloc(sizeof(sockaddr_in));
        if (!m_server) {
            DebugPrint("Failed to allocate space for server sockaddr_in.");
            closesocket(m_socket);
            WSACleanup();
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
			return false;
        }

        m_client = malloc(sizeof(sockaddr_in));
        if (!m_client) {
            DebugPrint("Failed to allocate space for client sockaddr_in.");
            closesocket(m_socket);
            WSACleanup();
            free(m_server);
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
			return false;
        }

        _srv->sin_addr.s_addr = INADDR_ANY;
        _srv->sin_family = AF_INET;
        _srv->sin_port = htons(port);

        if (bind(m_socket, (sockaddr*)_srv, sizeof(sockaddr_in)) == SOCKET_ERROR) {
            DebugPrintf("Failed to start winsock GDB server <%d>." , WSAGetLastError());
            closesocket(m_socket);
            WSACleanup();
            free(m_server);
            free(m_client);
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
			return false;
        }

        if (listen(m_socket, 3) == SOCKET_ERROR) {
            DebugPrintf("Failed to listen for connections to winsock GDB server <%d>." , WSAGetLastError());
            closesocket(m_socket);
            WSACleanup();
            free(m_server);
            free(m_client);
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
			return false;
        }

        DebugPrintf("Waiting for connection to winsock GDB server on port %d...", port);

        int c = sizeof(sockaddr_in);
        m_listening = true;
        Unlock();
        
        m_conn = accept(m_socket, (sockaddr*)_cli, &c);

        Lock();
        m_listening = false;
        if (m_conn == INVALID_SOCKET) {
            DebugPrintf("Failed to accept connection to winsock GDB server <%d>.", WSAGetLastError());
            if (m_socket) closesocket(m_socket);
            WSACleanup();
            free(m_server);
            free(m_client);
            m_conn = m_socket = 0;
			m_server = m_client = nullptr;
			Unlock();
            return false;
        }
        Unlock();

        GDB_Connected();
        return true;
    }

    void WinSockInterface::GDB_Connected() {
    }

    void WinSockInterface::IO_Close() {
        if (!m_server) {
            DebugPrint("Socket is not currently open.");
            return;
        }

        closesocket(m_socket);
        WSACleanup();
        free(m_server);

        if (m_client) free(m_client);

        m_conn = m_socket = 0;
        m_server = m_client = nullptr;

        DebugPrint("Closed GDB server.");
    }

    size_t WinSockInterface::IO_Peek() {
        if (!m_server) {
            DebugPrint("Socket is not currently open.");
            return 0;
        }

        unsigned long r;
        ioctlsocket(m_conn, FIONREAD, &r);
        return r;
    }

    Result WinSockInterface::IO_Read(void* dest, size_t size, size_t* bytesRead) {
        if (!m_server) {
            DebugPrint("Socket is not currently open.");
            return Result::InternalError;
        }

        *bytesRead = recv(m_conn, (char*)dest, size, 0);

        if (*bytesRead == 0) {
            return Result::InternalError;
		}

        return Result::Success;
    }

    Result WinSockInterface::IO_Write(const void* src, size_t size) {
        static char pkt[4096];
        if (size < 4096) {
            memcpy(pkt, src, size);
            pkt[size] = 0;
			DebugPrintf("Sent: %s\n", &pkt[0]);
        } else {
		    DebugPrintf("Packet too large for debug log buffer: %llu\n", size);
        }

        if (!m_server) {
            DebugPrint("Socket is not currently open.");
            return Result::InternalError;
        }

        if (size == 0) {
            if (send(m_conn, "", 1, 0) == 0) {
                return Result::InternalError;
            }
        }
        else if (send(m_conn, (const char*)src, size, 0) == 0) {
            return Result::InternalError;
        }

        return Result::Success;
    }

    Result WinSockInterface::IO_Poll() {
        if (!m_server) {
            DebugPrint("Socket is not currently open.");
            return Result::InternalError;
        }

        return Result::Success;
	}

	bool WinSockInterface::IsListening() const {
        return m_listening;
    }

	void WinSockInterface::StopListening() {
        Lock();
        closesocket(m_socket);
        m_socket = 0;
        Unlock();
    }
};
/*
int main (int argC, const char** argV) {
    GDB::WinSockInterface wsi(GDB::Architecture::MIPS);
    wsi.DefineRegister("zero", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("at", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("v0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("v1", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("a0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("a1", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("a2", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("a3", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("t0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t1", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t2", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t3", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t4", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t5", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t6", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t7", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("s0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s1", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s2", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s3", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s4", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s5", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s6", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("s7", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("t8", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("t9", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("k0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("k1", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("gp", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("sp", 32, GDB::RegisterType::StackPointer);

    wsi.DefineRegister("s8", 32, GDB::RegisterType::GeneralPurpose);

    wsi.DefineRegister("ra", 32, GDB::RegisterType::CodePointer);

    wsi.DefineRegister("f0", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f1", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f2", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f3", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f4", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f5", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f6", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f7", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f8", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f9", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f10", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f11", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f12", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f13", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f14", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f15", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f16", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f17", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f18", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f19", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f20", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f21", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f22", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f23", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f24", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f25", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f26", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f27", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f28", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f29", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f30", 32, GDB::RegisterType::GeneralPurpose);
    wsi.DefineRegister("f31", 32, GDB::RegisterType::GeneralPurpose);

    wsi.Enable(6169);

    if (wsi.Run() != GDB::Result::Success) {
        wsi.DebugPrint("Run exited with non-success status");
    }

    wsi.Disable();
}
*/