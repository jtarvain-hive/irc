#pragma once
#include <cstddef>

class Server;
struct Message;

typedef void (*Handler)(Server&, int, const Message&);

struct Command {
    Handler fn;
    bool    needsRegistration;
    size_t  minParams;
};
