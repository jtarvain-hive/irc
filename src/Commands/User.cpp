#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdUser(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return server.sendToClient(fd, ":ircserv 462 " + c->replyNick() + " :You may not reregister\r\n");

    // USER <username> <hostname> <servername> :<realname>, only the username is used
    c->setUser(msg.params[0]);
    server.tryRegister(fd);
}
