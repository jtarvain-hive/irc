#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdKick(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.size() < 2)
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " KICK :Not enough parameters\r\n");

    const std::string& channel = msg.params[0];
    const std::string& target  = msg.params[1];
    const std::string  reason  = msg.trailing.empty() ? target : msg.trailing;

    Channel* ch = server.getChannel(channel);
    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + channel + " :You're not on that channel\r\n");

    if (!ch->isOperator(fd))
        return server.sendToClient(fd, ":ircserv 482 " + c->getNick() + " " + channel + " :You're not a channel operator\r\n");

    int targetFd = server.findFdByNick(target);
    if (targetFd < 0 || !ch->hasMember(targetFd))
        return server.sendToClient(fd, ":ircserv 441 " + c->getNick() + " " + target + " " + channel + " :They aren't on that channel\r\n");

    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " KICK " << channel << " " << target;
    if (!msg.trailing.empty()) ss << " :" << msg.trailing;
    ss << "\r\n";
    server.sendToChannel(channel, ss.str());
    // flush kicked user's buffer so client processes KICK promptly
    int tgtFd = server.findFdByNick(target);
    if (tgtFd >= 0) server.flushToClient(tgtFd);
    server.removeMemberFromChannel(channel, target);
}
