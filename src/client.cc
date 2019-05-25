#include "p2pclient.hpp"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <CONNECT UDP Address> <CONNECT UDP port>\n", argv[0]);
        exit(1);
    }

    debug << "debug mode" << std::endl;
    std::string addr(argv[1]);
    int port = atoi(argv[2]);
    UdpClient client(port, addr);
    client.run();

    return 0;
    
}
