#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include "global.h"
#include "base.h"
#include "udpp2p.pb.h"
#include "srvhandle.hpp"

class UdpServer : public BaseP2P
{
protected:
    void initServer(int port,
                    std::string addr);
    void recvMsg();
    virtual void setDaemon() {};

protected:
    struct sockaddr_in m_addr;
    BaseHandler *m_ptrHandler;
    int m_fd;

public:
    UdpServer(int port = 12345,
              std::string addr = "0.0.0.0");
    virtual ~UdpServer() {};
    virtual int getSockfd()
    {
        return this->m_fd;
    }
    void run()
    {
        this->recvMsg();
    }
};

void UdpServer::initServer(int port, std::string addr)
{

    memset(&m_addr, 0, sizeof(struct sockaddr_in));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(addr.c_str());

    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    active_nonblock(m_fd);

    if (m_fd == -1)
        handle_error("socket");

    int on = 1;

    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &on,
                   sizeof(on)) < 0)
        handle_error("setsockopt SO_REUSEADDR");

    if (bind(m_fd, (struct sockaddr *)&m_addr,
             sizeof(struct sockaddr_in)) < 0)
        handle_error("bind");

    sigignore(SIGPIPE);

    debug << "initServer end" << std::endl;
}

UdpServer::UdpServer(int port, std::string addr) : BaseP2P()
{
    m_ptrHandler = BaseHandler::getInstance();
    m_ptrHandler->setBaseServer(this);
    initServer(port, addr);
    debug << "UdpServer end" << std::endl;
}

void UdpServer::recvMsg()
{
    debug << "recvMsg start" << std::endl;

    struct pollfd allfd[1];

    allfd[0].events = POLLIN;
    allfd[0].fd = this->m_fd;

    char recvBuf[MAXDATASIZE];
    udpp2p::CltReq message;

    while (1)
    {
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        // timewait: -1:block 0:call return >0:set timeout
        debug << "poll..." << std::endl;
        int ret = poll(allfd, 1, 5 * 1000);

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // Idle状态，执行心跳包等常规定时事件
        if (ret == 0)
        {

            udpp2p::CltReq nullReq;
            std::pair<char *, uint32_t> peerTuple;
            struct sockaddr addr;

            m_ptrHandler->matchAndHandle(udpp2p::MsgType::PONG,
                                         nullReq, peerTuple,
                                         addr);
        }

        if (allfd[0].revents & POLLIN)
        {
            struct sockaddr peerAddr;
            socklen_t peerAddrSize = sizeof(struct sockaddr_in);
            memset(&peerAddr, 0, sizeof(peerAddrSize));

            int recvNum = recvfrom(this->m_fd,
                                   &recvBuf,
                                   MAXDATASIZE,
                                   0,
                                   &peerAddr,
                                   &peerAddrSize);

            debug << "recvfrom..." << std::endl;

            auto peerTuple = get_addr_tuple(&peerAddr);

            debug << "cli-ip:" << peerTuple.first << " cli-port:" << peerTuple.second << std::endl;

            if (recvNum <= 0)
                continue;
            else
                recvBuf[recvNum] = '\0';

            if (message.ParseFromArray(&recvBuf, recvNum))
            {
                debug << "Recv Type:" << message.type() << std::endl;
                m_ptrHandler->matchAndHandle(message.type(), message, peerTuple, peerAddr);
            }
            else
            {
                std::cout << "Unknow Type, Parse Fail." << std::endl;
            }
        }
    }
}
