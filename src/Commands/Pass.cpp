#include "Command.hpp"
#include "../../include/Server.hpp"
#include "../../include/Client.hpp"

void cmdPass(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return server.sendToClient(fd, ":ircserv 462 * :You may not reregister\r\n");

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 * PASS :Not enough parameters\r\n");

    c->setPass(msg.params[0]);
    server.sendToClient(fd, ":ircserv NOTICE * :PASS command received\r\n");
    server.tryRegister(fd);
}
