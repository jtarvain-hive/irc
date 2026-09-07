#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

void cmdQuit(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c)
        return;

    std::string reason = msg.trailing;
    if (reason.empty())
        reason = "Client Quit";

    std::stringstream ss;
    ss << ":" << (c->getNick().empty() ? std::string("ircserv") : c->getNick())
       << "!~" << c->getUser() << "@" << c->getIpAddr()
       << " QUIT :" << reason << "\r\n";

    server.broadcastToMemberChannels(fd, ss.str());
    server.flushToClient(fd);
    server.disconnectClient(fd);
}