#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <thread>
#include <memory>
#include <sstream>
#include "global.h"
#include "base.h"
#include "udpp2p.pb.h"
#include "clthandle.hpp"

class UdpClient : public BaseP2P
{
protected:
    void initSocket(int port, std::string addr);
    void recvMsg();
    void menu();
    void printMenu();
    void p2pMode();
    void normalMode();
    void sendP2PReq();

protected:
    struct sockaddr_in m_srvAddr;
    BaseHandler *m_ptrHandler;
    std::unique_ptr<std::thread> sendMsgTrd;
    std::unique_ptr<std::thread> sendP2PTrd;
    int m_fd;

public:
    UdpClient(int port,
              std::string addr);
    virtual ~UdpClient(){};
    virtual int getSockfd()
    {
        return this->m_fd;
    }
    void run()
    {
        this->recvMsg();
    }
};

void UdpClient::initSocket(int port, std::string addr)
{
    memset(&m_srvAddr, 0, sizeof(struct sockaddr_in));
    m_srvAddr.sin_family = AF_INET;
    m_srvAddr.sin_port = htons(port);
    m_srvAddr.sin_addr.s_addr = inet_addr(addr.c_str());

    m_fd = socket(AF_INET, SOCK_DGRAM, 0);

    active_nonblock(m_fd);

    if (m_fd == -1)
        handle_error("socket");

    int on = 1;

    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &on,
                   sizeof(on)) < 0)
        handle_error("setsockopt SO_REUSEADDR");

    sigignore(SIGPIPE);
}

UdpClient::UdpClient(int port, std::string addr) : BaseP2P()
{
    getState() = 0x0;
    m_ptrHandler = BaseHandler::getInstance();
    m_ptrHandler->setBaseServer(this);
    initSocket(port, addr);
    sendMsgTrd = std::unique_ptr<std::thread>(new std::thread(&UdpClient::printMenu, this));
    sendP2PTrd = std::unique_ptr<std::thread>(new std::thread(&UdpClient::sendP2PReq, this));

    menu();
}

void UdpClient::recvMsg()
{
    struct pollfd allfd[1];

    allfd[0].events = POLLIN;
    allfd[0].fd = this->m_fd;

    char recvBuf[MAXDATASIZE];
    udpp2p::SrvRsp message;

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
            udpp2p::SrvRsp nullRsp;
            std::pair<char *, uint32_t> peerTuple;
            struct sockaddr addr;
            memcpy(&addr, &m_srvAddr, sizeof(struct sockaddr));

            m_ptrHandler->matchAndHandle(udpp2p::MsgType::PING,
                                         nullRsp, peerTuple,
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

            auto peerTuple = get_addr_tuple(&peerAddr);

            debug << "peer-ip:" << peerTuple.first << " peer-port:" << peerTuple.second << std::endl;

            if (recvNum <= 0)
                continue;
            else
                recvBuf[recvNum] = '\0';

            if (message.ParseFromArray(&recvBuf, recvNum))
            {
                debug << "Recv Type:" << message.type() << std::endl;
                //this->m_state |= message.type();
                m_ptrHandler->matchAndHandle(message.type(), message, peerTuple, peerAddr);
            }
            else
            {
                //this->getState = -1;
                std::cout << "Enter <help> to show menu." << std::endl;
            }
        }
    }
}

void UdpClient::p2pMode()
{

    std::cout << "[Enter P2P Chat Mode, Send Message]:";
    //////////////////
    std::string message;
    std::getline(std::cin, message);

    /* 发送消息 */
    udpp2p::SrvRsp msgReq;
    msgReq.set_type(udpp2p::MsgType::P2PREQ);
    msgReq.set_message(message);

    /* peer 信息初始化 */
    std::pair<char *, uint32_t> peerTuple;
    struct sockaddr peerAddr;
    struct sockaddr_in tempAddr;

    tempAddr.sin_family = AF_INET;
    memcpy(&tempAddr.sin_port, ((char *)&getState()) + 16,
           sizeof(uint16_t));
    memcpy(&tempAddr.sin_addr.s_addr, ((char *)&getState()) + 32,
           sizeof(uint32_t));
    memcpy(&peerAddr, &tempAddr, sizeof(struct sockaddr_in));

    ClientP2PSendHandler::getHandler()->handle(msgReq, peerTuple, peerAddr);
}

void UdpClient::menu()
{
    std::cout << "=======================Command Menu=======================" << std::flush;
    std::cout << "\n  help: Show this information."
                 "\n  login: Login p2p server to make you punchable."
                 "\n  list: List all client-id<cid> from server."
                 "\n  punch <cid>: Send punch request to peer client and start a p2p session."
              << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << "[Choose One Option]:" << std::flush;
}

void UdpClient::normalMode()
{

    while (std::cin.peek() == '\n' || std::cin.peek() == ' ')
    {
        std::cout << "[Choose One Option]:" << std::flush;
        std::cin.get();
        continue;
    }
    ////////////////////
    std::string command, option, cid;
    std::getline(std::cin, command);

    std::stringstream parse(command);
    parse >> option;
    parse >> cid;
    //std::cin.ignore(1024, '\n');

    if (!(getState() & udpp2p::MsgType::PONG))
    {
        std::cout << "\033[31m[Tip]:"
                  << "Connect to server fail. Wait and try again."
                  << "\033[0m" << std::endl;
        std::cout << "[Choose One Option]:" << std::flush;
        return;
    }

    switch (hash_(option.c_str()))
    {
    case "help"_hash:
        menu();
        break;
    case "login"_hash:
        std::cout << "\033[34m[Tip]:"
                  << "Login success, conn to server."
                  << "\033[0m" << std::endl;
        break;
    case "list"_hash:
        debug << "list" << std::endl;
        {
            udpp2p::CltReq nullReq;
            std::pair<char *, uint32_t> peerTuple;
            struct sockaddr addr;
            memcpy(&addr, &m_srvAddr, sizeof(struct sockaddr));
            PullSendHandler::getHandler()->handle(nullReq, peerTuple, addr);
        }
        break;
    case "punch"_hash:
    {
        size_t id = atoi(cid.c_str());
        if (getMap().find(id) != getMap().end())
        {
            udpp2p::CltReq nullReq;
            char *peerIp = const_cast<char *>(getMap()[id].first.c_str());
            uint32_t peerPort = getMap()[id].second;
            std::pair<char *, uint32_t> peerTuple(peerIp, peerPort);

            struct sockaddr addr;
            memcpy(&addr, &m_srvAddr, sizeof(struct sockaddr));

            ClientLinkSendHandler::getHandler()->handle(nullReq, peerTuple, addr);
            std::cout << "\033[34m[Tip]:"
                      << "Request success, please wait 5s and enter <p2p>."
                      << "\033[0m" << std::endl;
        }
        else
        {

            std::cout << "\033[31m[Tip]:"
                      << "cid error, please enter <list> command."
                      << "\033[0m" << std::endl;
        }
    }
    break;
    default:
        // if (option.size() != 0)
        std::cout << "\033[31m[Tip]:"
                  << "!!Unkown Options!!"
                  << "\033[0m" << std::endl;
        break;
    }
}

void UdpClient::printMenu()
{

    std::string command, option, cid;
    std::stringstream parse;

    while (1)
    {
        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        /* Watch stdin (fd 0) to see when it has input. */
        int retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval > 0)
        {
            if (getState() & BITSET(0x0E))
            {
                p2pMode();
            }
            else
            {
                normalMode();
            }
        }
    }
}

void UdpClient::sendP2PReq()
{
    while (1)
    {

        if (getState() & BITSET(0x0F))
        {

            // struct in_addr t = {ip};
            // debug << "\033[31m[Tip]:"
            //       << "port:" << ntohs(port)
            //       << " addr:" << inet_ntoa(t)
            //       << "\033[0m" << std::endl;

            udpp2p::SrvRsp nullReq;
            std::pair<char *, uint32_t> peerTuple;
            struct sockaddr peerAddr;
            struct sockaddr_in tempAddr;

            tempAddr.sin_family = AF_INET;
            memcpy(&tempAddr.sin_port, ((char *)&getState()) + 16,
                   sizeof(uint16_t));
            memcpy(&tempAddr.sin_addr.s_addr, ((char *)&getState()) + 32,
                   sizeof(uint32_t));
            memcpy(&peerAddr, &tempAddr, sizeof(struct sockaddr_in));

            ClientP2PSendHandler::getHandler()->handle(nullReq, peerTuple, peerAddr);
        }

        // debug << "\033[31m[Tip]:"
        //       << "getState"
        //       << ":" << (getState() & BITSET(0x0F)) << ":" << getState()
        //       << "\033[0m" << std::endl;

        sleep(10);
    }
}