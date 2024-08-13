#include "server_tcp.h"
#include "block_rnd.h"

int main(int argc, char **argv)
{
    std::unique_ptr<IServer> s = std::make_unique<ServerTcp>(std::make_shared<BlockRnd>());
    s->loop();
    return 0;
}
