#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdWho(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    const std::string target = msg.params.empty() ? std::string("*") : msg.params[0];

    server.sendToClient(fd, ":ircserv 315 " + c->getNick() + " " + target + " :End of WHO list\r\n");
}
