#ifndef __SERVER_HANDLE__H__P2P__
#define __SERVER_HANDLE__H__P2P__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "udpp2p.pb.h"
#include "global.h"
#include "base.h"

/////////////////////////////////////////////////////////////

class HeartManageHandler : public BaseHandler
{

private:
  friend class ReplyHandler; // 优化和心跳handler的关系，以便在无client 的情况下停止检查心跳
  static HeartManageHandler m_handler;
  HeartManageHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::PING);
    BaseHandler::addToHandlers(this);
  }

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
    if (user_status_map.size() == 0)
    {
      this->m_hanlerType &= ~BITSET(udpp2p::MsgType::PONG);
      return;
    }

    for (auto it = user_status_map.begin(); it != user_status_map.end(); ++it)
    {
      --it->second;
      debug << "second:->" << (int)it->second << std::endl;

      if (it->second == 0)
      {
        user_map.erase(it->first);
        user_status_map.erase(it);
      }

      debug << "online:" << user_map.size() << std::endl;
    }
  }
};

HeartManageHandler HeartManageHandler::m_handler;

/////////////////////////////////////////////////////////////

class ReplyHandler : public BaseHandler
{

protected:
  static ReplyHandler m_handler;
  ReplyHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::PING);
    BaseHandler::addToHandlers(this);
  }

public:
  ReplyHandler *getHandler()
  {
    return &m_handler;
  }

  std::string handlerName()
  {
    return "ReplyHandler";
  }

  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    debug << "ReplyHandler work..." << std::endl;

    udpp2p::CltReq msgReq;
    msgReq.CopyFrom(msg);

    debug << "ssid:" << msgReq.sessionid() << std::endl;
    // 加入上线列表,old 更新，new 插入
    {
      // std::lock_guard<std::mutex> lck(mtx);
      user_map.insert(std::make_pair(msgReq.sessionid(), peerTuple));
      user_status_map[msgReq.sessionid()] = HEARTTIMEOT;
      HeartManageHandler::m_handler.getHandler()->m_hanlerType |= BITSET(udpp2p::MsgType::PONG);
    }

    udpp2p::SrvRsp rsp;

    rsp.set_type(udpp2p::MsgType::PONG);

    std::unique_ptr<char> buffer(new char[rsp.ByteSize()]);
    rsp.SerializeToArray(buffer.get(), rsp.ByteSize());

    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           rsp.ByteSize(),
           0,
           &peerAddr,
           sizeof(struct sockaddr));
  }
};

ReplyHandler ReplyHandler::m_handler;

/////////////////////////////////////////////////////////////

class PushListHandler : public BaseHandler
{

private:
  static PushListHandler m_handler;
  PushListHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::PULL);
    BaseHandler::addToHandlers(this);
  }

public:
  PushListHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "PushListHandler";
  }
  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    udpp2p::SrvRsp rsp;

    rsp.set_type(udpp2p::MsgType::PUSH);

    // rsp.set_sessionid(hash_code);

    for (auto it = user_map.begin(); it != user_map.end(); ++it)
    {
      auto addrs = rsp.add_addrs();
      addrs->set_sessionid(it->first);
      addrs->set_addr(it->second.first);
      addrs->set_port(it->second.second);
    }

    std::unique_ptr<char> buffer(new char[rsp.ByteSize()]);
    rsp.SerializeToArray(buffer.get(), rsp.ByteSize());

    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           rsp.ByteSize(),
           0,
           &peerAddr,
           sizeof(struct sockaddr));
  }
};

PushListHandler PushListHandler::m_handler;

/////////////////////////////////////////////////////////////

class ServerLinkReqHandler : public BaseHandler
{
private:
  static ServerLinkReqHandler m_handler;
  ServerLinkReqHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::LINKREQ);
    BaseHandler::addToHandlers(this);
  }

public:
  ServerLinkReqHandler *getHandler()
  {
    return &m_handler;
  }
  std::string handlerName()
  {
    return "ServerLinkReqHandler";
  }

  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {

    udpp2p::CltReq msgReq;
    msgReq.CopyFrom(msg);

    // 接受Client A的消息，转发给Client B
    udpp2p::CltReq req;

    req.set_type(udpp2p::MsgType::LINKREQ);

    auto reqAddr = req.add_addrs();
    reqAddr->set_sessionid(msgReq.sessionid());
    reqAddr->set_addr(peerTuple.first);
    reqAddr->set_port(peerTuple.second);

    std::unique_ptr<char> buffer(new char[req.ByteSize()]);
    req.SerializeToArray(buffer.get(), req.ByteSize());

    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(struct sockaddr_in));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(msgReq.addrs(0).port());
    sockAddr.sin_addr.s_addr = inet_addr(msgReq.addrs(0).addr().c_str());

    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           req.ByteSize(),
           0,
           (const struct sockaddr *)&sockAddr,
           sizeof(sockAddr));
  }
};

ServerLinkReqHandler ServerLinkReqHandler::m_handler;

/////////////////////////////////////////////////////////////

class ServerLinkRspHandler : public BaseHandler
{

private:
  static ServerLinkRspHandler m_handler;
  ServerLinkRspHandler()
  {
    this->m_hanlerType = BITSET(udpp2p::MsgType::LINKREQ);
    BaseHandler::addToHandlers(this);
  }

public:
  ServerLinkRspHandler *getHandler()
  {
    return &m_handler;
  }

  std::string handlerName()
  {
    return "ServerLinkRspHandler";
  }

  void handle(::google::protobuf::Message &msg,
              std::pair<char *, uint32_t> &peerTuple,
              struct sockaddr &peerAddr)
  {
    //此处特殊，修改报文类型后，直接将请求原路返回Client A
    udpp2p::CltReq rsp;
    rsp.CopyFrom(msg);
    rsp.set_type(udpp2p::MsgType::LINKRSP);

    std::unique_ptr<char> buffer(new char[rsp.ByteSize()]);
    rsp.SerializeToArray(buffer.get(), rsp.ByteSize());

    sendto(BaseHandler::base_server->getSockfd(),
           buffer.get(),
           rsp.ByteSize(),
           0,
           &peerAddr,
           sizeof(peerAddr));
  }
};

ServerLinkRspHandler ServerLinkRspHandler::m_handler;

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
#endif