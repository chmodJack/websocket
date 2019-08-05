#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#define SOCKET_DEBUG 0

#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>

#if WIN32

#include<winsock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

namespace coder1
{

    class udp_socket
    {
    public:
        udp_socket(void)
        {
            WSADATA wsa_data;
            if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
            {
                printf("error: %s %d\n", __FILE__, __LINE__);
                exit(-1);
            }
            m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (m_socket == -1)
            {
                printf("socket error !");
                WSACleanup();
                exit(-1);
            }
        }
        ~udp_socket(void)
        {
            closesocket(m_socket);
            WSACleanup();
        }
        bool set_block(bool state)
        {
            unsigned long ul = (state) ? 0 : 1;
            if (-1 == ioctlsocket(m_socket, FIONBIO, &ul))
            {
                printf("set_block error\n");
                return false;
            }
            return true;
        }
        bool bind(const char* ip, uint16_t port)
        {
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.S_un.S_addr);

            if (::bind(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)) == -1)
            {
                printf("bind error !\n");
                return false;
            }

            return true;
        }

        size_t recv(char* buffer, int len, char* ip, uint16_t* port)
        {
            sockaddr_in remote_addr;
            int addr_len = sizeof(sockaddr_in);

            size_t count = recvfrom(m_socket, buffer, len, 0, (sockaddr*)&remote_addr, &addr_len);
            if (count != -1)
            {
                if (ip)
                    inet_ntop(AF_INET, (void*)&(remote_addr.sin_addr), ip, 16);
                if (port)
                    *port = ntohs(remote_addr.sin_port);
            }
            else
            {
                #if SOCKET_DEBUG
                printf("recv error\n");
                #endif
            }

            return count;
        }
        size_t send(const char* buffer, size_t len, const char* ip, uint16_t port)
        {
            sockaddr_in remote_addr;
            int addr_len = sizeof(sockaddr_in);
            remote_addr.sin_family = AF_INET;
            remote_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&remote_addr.sin_addr.S_un.S_addr);

            size_t count = sendto(m_socket, buffer, (int)len, 0, (sockaddr *)&remote_addr, addr_len);
            if (count == -1)
            {
                #if SOCKET_DEBUG
                printf("send error\n");
                #endif
            }

            return count;
        }
    public:
        SOCKET m_socket;
    };

    class tcp_socket
    {
    public:
        tcp_socket(void)
        {
            WSADATA wsa_data;
            if (WSAStartup(MAKEWORD(1, 1), &wsa_data) != 0)
            {
                printf("error: %s %d\n", __FILE__, __LINE__);
                exit(-1);
            }
            m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_socket == -1)
            {
                printf("socket error !");
                WSACleanup();
                exit(-1);
            }
        }
        ~tcp_socket(void)
        {
            closesocket(m_socket);
            WSACleanup();
        }
        bool set_block(bool state)
        {
            unsigned long ul = (state) ? 0 : 1;
            return (-1 == ioctlsocket(m_socket, FIONBIO, &ul)) ? false : true;
        }
        bool bind(const char* ip, uint16_t port)
        {
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.S_un.S_addr);

            if (::bind(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)) == SOCKET_ERROR)
            {
                printf("bind error !\n");
                return false;
            }

            return true;
        }

        bool listen(int backlog)
        {
            if (-1 == ::listen(m_socket, backlog))
            {
                printf("listen error\n");
                return false;
            }
            return true;
        }
        SOCKET accept(void)
        {
            SOCKET new_sock = ::accept(m_socket, nullptr, nullptr);
            if (-1 == new_sock)
            {
                printf("accept error\n");
            }
            return new_sock;
        }
        bool connect(const char* ip, uint16_t port)
        {
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.S_un.S_addr);

            if (-1 == ::connect(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)))
            {
                printf("connect error\n");
                return false;
            }
            return true;
        }

    public:
        SOCKET m_socket;
    };

}

#endif

#if __linux

#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<fcntl.h>
#include<arpa/inet.h>

namespace coder1
{
    class udp_socket
    {
    public:
        udp_socket(void)
        {
            m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (m_socket == -1)
            {
                printf("socket error !");
                exit(-1);
            }
        }
        ~udp_socket(void)
        {
            close(m_socket);
        }
        bool set_block(bool state)
        {
            int flags = fcntl(m_socket, F_GETFL, 0);
            int ret = 0;

            if (state)
                ret = fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
            else
                ret = fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

            if (ret == 0)
                return true;
            else
                return false;
        }
        bool bind(const char* ip, uint16_t port)
        {
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.s_addr);

            if (::bind(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)) == -1)
            {
                printf("bind error !\n");
                return false;
            }

            return true;
        }

        size_t recv(char* buffer, int len, char* ip, uint16_t* port)
        {
            struct sockaddr_in remote_addr;
            unsigned int addr_len = sizeof(sockaddr_in);

            int count = recvfrom(m_socket, buffer, len, 0, (sockaddr*)&remote_addr, &addr_len);
            if (count != -1)
            {
                if (ip)
                    inet_ntop(AF_INET, (void*)&(remote_addr.sin_addr), ip, 16);
                if (port)
                    *port = ntohs(remote_addr.sin_port);
            }
            else
            {
                #if SOCKET_DEBUG
                printf("recv error\n");
                #endif
            }

            return count;
        }
        size_t send(const char* buffer, size_t len, const char* ip, uint16_t port)
        {
            sockaddr_in remote_addr;
            int addr_len = sizeof(sockaddr_in);
            remote_addr.sin_family = AF_INET;
            remote_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&remote_addr.sin_addr.s_addr);

            int count = sendto(m_socket, buffer, (int)len, 0, (sockaddr *)&remote_addr, addr_len);
            if (count == -1)
            {
                #if SOCKET_DEBUG
                printf("send error\n");
                #endif
            }

            return count;
        }
    public:
        int m_socket;
    };

    class tcp_socket
    {
    public:
        tcp_socket(void)
        {
            m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
            if (m_socket == -1)
            {
                printf("socket error !");
                exit(-1);
            }
        }
        ~tcp_socket(void)
        {
            close(m_socket);
        }
        bool set_block(bool state)
        {
            int flags = fcntl(m_socket, F_GETFL, 0);
            int ret = 0;

            if (state)
                ret = fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
            else
                ret = fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

            if (ret == 0)
                return true;
            else
                return false;
        }
        bool bind(const char* ip, uint16_t port)
        {
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.s_addr);

            if (::bind(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)) == -1)
            {
                printf("bind error !\n");
                return false;
            }

            return true;
        }

        bool listen(int backlog)
        {
            if (-1 == ::listen(m_socket, backlog))
            {
                printf("listen error\n");
                return false;
            }
            return true;
        }
        //struct sockaddr *addr, socklen_t *addrlen
        int accept(struct sockaddr* addr = nullptr, socklen_t* addrlen = nullptr)
        {
            int new_sock = ::accept(m_socket, addr, addrlen);
            if (-1 == new_sock)
            {
                printf("accept error\n");
            }
            return new_sock;
        }

        bool connect(const char* ip, uint16_t port)
        {
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip, (void*)&server_addr.sin_addr.s_addr);

            if (-1 == ::connect(m_socket, (sockaddr *)&server_addr, sizeof(sockaddr)))
            {
                printf("connect error\n");
                return false;
            }
            return true;
        }
    public:
        int m_socket;
    };

}

#endif

#endif//__SOCKET_HPP__

