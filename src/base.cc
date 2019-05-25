#include "base.h"

////////////////////////////////////////////////////////////

BaseHandler *BaseHandler::m_handlers[];
int BaseHandler::_nextSlot;

BaseHandler BaseHandler::base_handler;
BaseP2P *BaseHandler::base_server;

std::map<uint64_t, std::pair<std::string, uint32_t>> BaseHandler::user_map;
std::map<uint64_t, int8_t> BaseHandler::user_status_map;

void BaseHandler::setBaseServer(BaseP2P *server)
{
  base_server = server;
}

BaseHandler *BaseHandler::getInstance()
{
  return &base_handler;
}

void BaseHandler::matchAndHandle(udpp2p::MsgType type,::google::protobuf::Message &msg,
                                 std::pair<char *, uint32_t> &peerTuple,
                                 struct sockaddr &peerAddr)
{
  for (int i = 0; i < _nextSlot; ++i)
  {
    if (m_handlers[i]->matchType(type))
    {
      debug << "match Type:" << type << ",handler->" << m_handlers[i]->handlerName() << std::endl;
      m_handlers[i]->handle(msg, peerTuple, peerAddr);
    }
  }
}

bool BaseHandler::matchType(udpp2p::MsgType type)
{
  return this->m_hanlerType & BITSET(type);
}

void BaseHandler::handle(::google::protobuf::Message &msg,
                         std::pair<char *, uint32_t> &peerTuple,
                         struct sockaddr &peerAddr)
{
  return;
}

void BaseHandler::addToHandlers(BaseHandler *handler)
{
  debug << (void *)handler << std::endl;
  m_handlers[_nextSlot++] = handler;
}

std::string BaseHandler::handlerName()
{
  return "BaseHandler";
}
