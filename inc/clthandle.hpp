#ifndef __CLIENT_HANDLE__H__P2P__
#define __CLIENT_HANDLE__H__P2P__

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "udpp2p.pb.h"
#include "global.h"
#include "base.h"

/////////////////////////////////////////////////////////////

class HeartManageHandler : public BaseHandler
{
protected:
  uint8_t m_srvState = HEARTTIMEOT;

private:
  friend class ReplyHandler; // 优化和心跳handler的关系，以便在无client 的情况下停止检查心跳
  static HeartManageHandler m_handler;
  HeartManageHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::PONG);
    BaseHandler::addToHandlers(this);
  }
  friend class EchoHandler;

public:
  HeartManageHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "HeartManageHandler";
  }

  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {

    BaseHandler::base_server->getState() |= udpp2p::MsgType::PONG;
    //this->m_hanlerType |= BITSET(udpp2p::MsgType::PING);
    this->m_srvState = HEARTTIMEOT;
  }
};

HeartManageHandler HeartManageHandler::m_handler;

/////////////////////////////////////////////////////////////

class EchoHandler : public BaseHandler
{
private:
  static EchoHandler m_handler;
  EchoHandler()
  {
    // Handler订阅的事件如下，接受到这些事件都会执行
    this->m_hanlerType = BITSET(udpp2p::MsgType::PING);
    BaseHandler::addToHandlers(this);
  }

public:
  EchoHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "EchoHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    --HeartManageHandler::m_handler.getHandler()->m_srvState;
    if (HeartManageHandler::m_handler.getHandler()->m_srvState == 0)
    {
      BaseHandler::base_server->getState() &= ~udpp2p::MsgType::PONG;
    }

    udpp2p::CltReq req;

    req.set_type(udpp2p::MsgType::PING);

    debug << "gen_machine_code():" << ::gen_machine_code() << std::endl;

    req.set_sessionid(::gen_machine_code());
    std::unique_ptr<char> buffer(new char[req.ByteSize()]);
    req.SerializeToArray(buffer.get(), req.ByteSize());
    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           req.ByteSize(),
           0,
           &peerAddr,
           sizeof(peerAddr));
  }
};

EchoHandler EchoHandler::m_handler;

/////////////////////////////////////////////////////////////

class PullSendHandler : public BaseHandler
{
private:
  static PullSendHandler m_handler;
  PullSendHandler()
  {
    this->m_hanlerType = INVALID;
    BaseHandler::addToHandlers(this);
  }

public:
  static PullSendHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "PullSendHandler";
  }

  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    debug << this->handlerName() << std::endl;

    udpp2p::CltReq req;

    req.set_type(udpp2p::MsgType::PULL);
    req.set_sessionid(::gen_machine_code());

    std::unique_ptr<char> buffer(new char[req.ByteSize()]);
    req.SerializeToArray(buffer.get(), req.ByteSize());
    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           req.ByteSize(),
           0,
           &peerAddr,
           sizeof(peerAddr));
  }
};

PullSendHandler PullSendHandler::m_handler;

/////////////////////////////////////////////////////////////

class PullRecvHandler : public BaseHandler
{

private:
  static PullRecvHandler m_handler;
  PullRecvHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::PUSH);
    BaseHandler::addToHandlers(this);
  }

public:
  PullRecvHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "PullRecvHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {

    BaseHandler::base_server->getMap().clear();

    udpp2p::SrvRsp msgRsp;
    msgRsp.CopyFrom(msg);

    for (auto it = 0; it < msgRsp.addrs_size(); ++it)
    {
      auto addr = msgRsp.addrs(it);
      if (addr.sessionid() != ::gen_machine_code())
      {
        std::cout << std::internal << std::setw(20) << "\033[33m"
                  //<< "ssid[" << addr.sessionid() << "]\t"
                  << "cid:" << it << "\t"
                  << addr.addr() << ":" << addr.port()
                  << "\033[0m" << std::endl;
        BaseHandler::base_server->getMap().insert(std::make_pair(it, std::make_pair(addr.addr(), addr.port())));
      }
    }

    debug << "!!!!!!!!!!!!:" << BaseHandler::base_server->getMap().size() << std::endl;
    if (0 == BaseHandler::base_server->getMap().size())
    {
      std::cout << std::internal << std::setw(20) << "\033[33m"
                << "Sorry! No other user online."
                << "\033[0m" << std::endl;
    }

    // std::unique_ptr<char> buffer(new char[rsp.ByteSize()]);
    // rsp.SerializeToArray(buffer.get(), rsp.ByteSize());

    // sendto(BaseHandler::base_server->getSockfd(),
    //        buffer.get(),
    //        rsp.ByteSize(),
    //        0,
    //        &peerAddr,
    //        sizeof(struct sockaddr));
  }
};

PullRecvHandler PullRecvHandler::m_handler;

/////////////////////////////////////////////////////////////

class ClientLinkSendHandler : public BaseHandler
{
private:
  static ClientLinkSendHandler m_handler;
  ClientLinkSendHandler()
  {
    this->m_hanlerType = INVALID;
    BaseHandler::addToHandlers(this);
  }

public:
  static ClientLinkSendHandler *getHandler()
  {
    return &m_handler;
  }

  std::string handlerName()
  {
    return "ClientLinkSendHandler";
  }

  void handle(::google::protobuf ::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    debug << "\033[34m[Tip]:"
          << "ClientLinkSendHandler work"
          << "\033[0m" << std::endl;

    udpp2p::CltReq req;

    req.set_type(udpp2p::MsgType::LINKREQ);
    req.set_sessionid(::gen_machine_code());

    auto addrs = req.add_addrs();
    addrs->set_sessionid(INVALID);
    addrs->set_addr(peerTuple.first);
    addrs->set_port(peerTuple.second);

    std::unique_ptr<char> buffer(new char[req.ByteSize()]);
    req.SerializeToArray(buffer.get(), req.ByteSize());
    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           req.ByteSize(),
           0,
           &peerAddr,
           sizeof(peerAddr));
  }
};

ClientLinkSendHandler ClientLinkSendHandler::m_handler;

/////////////////////////////////////////////////////////////

class ClientLinkRecvHandler : public BaseHandler
{
private:
  static ClientLinkRecvHandler m_handler;
  ClientLinkRecvHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::LINKREQ) | BITSET(udpp2p::MsgType::LINKRSP);
    BaseHandler::addToHandlers(this);
  }

public:
  ClientLinkRecvHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "ClientLinkRecvHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {

    udpp2p::SrvRsp msgRsp;
    msgRsp.CopyFrom(msg);

    /* 当m_state 此位置为1发送P2P Req */
    BaseHandler::base_server->getState() |= BITSET(0x0F);
    /* 转为网络字节序后，填充ip，port */
    uint16_t port = htons(msgRsp.addrs(0).port());
    uint32_t ip = inet_addr(msgRsp.addrs(0).addr().c_str());
    memcpy(((char *)&BaseHandler::base_server->getState()) + 16,
           &port, sizeof(uint16_t));

    memcpy(((char *)&BaseHandler::base_server->getState()) + 32,
           &ip, sizeof(uint32_t));
  }
};

ClientLinkRecvHandler ClientLinkRecvHandler::m_handler;

/////////////////////////////////////////////////////////////
class ClientP2PSendHandler : public BaseHandler
{
private:
  static ClientP2PSendHandler m_handler;
  ClientP2PSendHandler()
  {
    this->m_hanlerType = INVALID;
    BaseHandler::addToHandlers(this);
  }

public:
  static ClientP2PSendHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "ClientP2PSendHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    /* 由于udpClient接受格式为SrvRsp格式，所以构造一个该请求 */
    udpp2p::SrvRsp req;
    req.CopyFrom(msg);

    req.set_type(udpp2p::MsgType::P2PREQ);
    req.set_sessionid(::gen_machine_code());

    std::unique_ptr<char> buffer(new char[req.ByteSize()]);
    req.SerializeToArray(buffer.get(), req.ByteSize());
    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           req.ByteSize(),
           0,
           &peerAddr,
           sizeof(peerAddr));
  }
};

ClientP2PSendHandler ClientP2PSendHandler::m_handler;

/////////////////////////////////////////////////////////////
class ClientP2PRecvHandler : public BaseHandler
{
private:
  static ClientP2PRecvHandler m_handler;
  ClientP2PRecvHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::P2PREQ);
    BaseHandler::addToHandlers(this);
  }

public:
  ClientP2PRecvHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "ClientP2PRecvHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    /* 当m_state 此位置为1说明打洞成功 */
    BaseHandler::base_server->getState() |= BITSET(0x0E);

    /* 解析请求消息 */
    udpp2p::SrvRsp msgRsp;
    msgRsp.CopyFrom(msg);

    if (msgRsp.has_message())
    {
      std::cout << std::endl;
      std::cout << "【Enter P2P Chat Mode, Recv Message】=>" << msgRsp.message() << std::endl;
      //std::cin.putback('\n');
    }
  }
};

ClientP2PRecvHandler ClientP2PRecvHandler::m_handler;

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
#endif