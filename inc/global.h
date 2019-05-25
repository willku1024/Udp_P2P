#ifndef __GLOBAL__H__P2P__
#define __GLOBAL__H__P2P__

/////

#include <utility>
#include <iomanip>
#define DEBUG1
#if defined(DEBUG)
#define debug std::cerr<<std::left<<std::setw(20)<<__FILE__<<std::right<<"["<<std::setw(5)<<__LINE__<<"]:\t"
#else
#define debug std::cerr.setstate(std::ios::failbit);std::cerr
#endif

#define BITSET(num) (1 << (num))
#define INVALID 0x00       // 定义一个无效的标志
#define HEARTTIMEOT 0x05   //可容忍心跳超时次数
#define MAXDATASIZE 0x1000 // UDP接受buffer大小
#define MAXHANDLERS 0x10

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

using hash_t = char;

///////////////////////////////////////////////////////////// function decl

extern std::pair<char *, unsigned int> get_addr_tuple(const struct sockaddr *remote_addr);
extern unsigned long gen_machine_code();
extern hash_t hash_(char const *str);

///////////////////////////////////////////////////////////// util function

constexpr hash_t operator""_hash(const char *str, size_t size)
{
    unsigned int hash = 0;
    for (int i = 0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }

    return (hash & 0x7FFFFFFF);
}
#endif