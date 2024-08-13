#include "client_tcp.h"
#include "data_list_rnd.h"

static unsigned dataListSize(int argc, char **argv)
{
    unsigned sz = 1;
    if (argc > 1) {
        try {
            sz = std::stoul(argv[1], nullptr, 10);
        } catch (std::exception &ex) {
            (void)ex;
        }
    }
    return sz;
}

int main(int argc, char **argv)
{
    auto dl = std::make_shared<DataListRnd>();
    dl->setSize(::dataListSize(argc, argv));
    std::unique_ptr<IClient> c = std::make_unique<ClientTcp>(dl);
    c->getData();
    return 0;
}
