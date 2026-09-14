#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdPing(Server& server, int fd, const Message& msg)
{
    std::string token;
    if (!msg.trailing.empty())
        token = msg.trailing;
    else if (!msg.params.empty())
        token = msg.params[0];

    server.sendToClient(fd, ":ircserv PONG ircserv :" + token + "\r\n");
}
