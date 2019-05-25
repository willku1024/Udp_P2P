#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstddef>
#include <ctime>
#include <functional>
#include <stdint.h>
#include "global.h"
///////////////////////////////////////////////////////////// util function

std::pair<char *, unsigned int> get_addr_tuple(const struct sockaddr *remote_addr)
{
    unsigned int rt_port = ntohs(((const struct sockaddr_in *)remote_addr)->sin_port);
    char *rt_addr = inet_ntoa(((const struct sockaddr_in *)remote_addr)->sin_addr);
    return std::make_pair(rt_addr, rt_port);
}

unsigned long gen_machine_code()
{
    const static unsigned long ssid = std::hash<time_t>()(time(NULL) * clock());
    return ssid;
}

hash_t hash_ (char const *str)
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
