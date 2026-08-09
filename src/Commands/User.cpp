#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdUser(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return server.sendToClient(fd, ":ircserv 462 * :You may not reregister\r\n");

    // <username> <hostname> <servername> <realname>
    // We care about the username and realname.
    const std::string& user = msg.params[0];
    std::string realname = msg.trailing;
    if (realname.empty() && msg.params.size() >= 4)
        realname = msg.params[3];

    c->setUser(user, realname);
    server.tryRegister(fd);
}
