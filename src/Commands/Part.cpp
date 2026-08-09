#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdPart(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " PART :Not enough parameters\r\n");

    const std::string& name = msg.params[0];

    Channel* ch = server.getChannel(name);
    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + name + " :You're not on that channel\r\n");

    std::string reason = msg.trailing;
    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " PART " << name;
    if (!reason.empty()) ss << " :" << reason;
    ss << "\r\n";
    server.sendToChannel(name, ss.str());
    // ensure client receives the PART immediately
    server.flushToClient(fd);
    server.removeMemberFromChannel(name, c->getNick());
}
