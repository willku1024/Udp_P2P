#include "p2pserver.hpp"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <Listen UDP Address> <Listen UDP port>\n", argv[0]);
        exit(1);
    }

    debug << "debug mode" << std::endl;
    std::string addr(argv[1]);
    int port = atoi(argv[2]);
    UdpServer server(port, addr);
    server.run();

    return 0;
    
}
