#ifndef __BASE__H__P2P__
#define __BASE__H__P2P__

#include <iostream>
#include <map>
#include "global.h"
#include "udpp2p.pb.h"
////////////////////////////////////////////////////////////

class BaseP2P
{
private:
    std::map<uint64_t, std::pair<std::string, uint32_t>> m_user_map;
    uint64_t m_state;

public:
    /* 非必须实现 */
    virtual std::map<uint64_t, std::pair<std::string, uint32_t>> &
    getMap()
    {
        // 可以用于传出一个用户相关的map
        return m_user_map;
    };
    virtual uint64_t &getState()
    {
        /**
        * m_state 可以用于传出一些自定义状态
        * m_state 0-11位:
        *         获取网络层的通讯状态，即与udp2p::MsgType对应
        * m_state 12-15位: 
        *         第15位：是否开启了sendP2PReq() 内网穿透线程，准备打洞
        *         第14位: 是否收到p2preq，说明打洞成功
        *         其他：暂未使用
        * m_state 16-31位
        *         p2p中对端的port 
        * m_state 32-63位
        *         p2p中对端的ip  
        */
        return m_state;
    };

    /* 必须实现 */
    virtual int getSockfd() = 0; // 传出继承者的sockfd
};

/////////////////////////////////////////////////////////////

class BaseHandler
{

public:
    uint16_t m_hanlerType;
    static BaseHandler *m_handlers[MAXHANDLERS];
    static int _nextSlot;

protected:
    virtual void handle(::google::protobuf::Message &msg,
                        std::pair<char *, uint32_t> &peerTuple,
                        struct sockaddr &peerAddr);

    static void addToHandlers(BaseHandler *Handler);

public:
    virtual ~BaseHandler() {}

    virtual std::string handlerName();
    virtual bool matchType(udpp2p::MsgType type);
    static void matchAndHandle(udpp2p::MsgType type, ::google::protobuf::Message &msg,
                               std::pair<char *, uint32_t> &peerTuple,
                               struct sockaddr &peerAddr);

    static BaseHandler *getInstance();

    static void setBaseServer(BaseP2P *server);

protected:
    static BaseHandler base_handler;
    static BaseP2P *base_server;
    static std::map<uint64_t, std::pair<std::string, uint32_t>> user_map;
    static std::map<uint64_t, int8_t> user_status_map;

public:
    BaseHandler() = default;
    BaseHandler(const BaseHandler &) = delete;
    BaseHandler &operator=(const BaseHandler &) = delete;
};

#endif