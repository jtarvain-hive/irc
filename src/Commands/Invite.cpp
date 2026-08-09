#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdInvite(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.size() < 2)
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " INVITE :Not enough parameters\r\n");

    const std::string& target  = msg.params[0];
    const std::string& channel = msg.params[1];

    int targetFd = server.findFdByNick(target);
    if (targetFd < 0)
        return server.sendToClient(fd, ":ircserv 401 " + c->getNick() + " " + target + " :No such nick\r\n");

    Channel* ch = server.getChannel(channel);
    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + channel + " :You're not on that channel\r\n");

    if (!ch->isOperator(fd))
        return server.sendToClient(fd, ":ircserv 482 " + c->getNick() + " " + channel + " :You're not a channel operator\r\n");

    server.addInvite(channel, target);
    server.sendToClient(fd, ":ircserv 341 " + c->getNick() + " " + target + " " + channel + "\r\n");

    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " INVITE " << target << " :" << channel << "\r\n";
    server.sendToClient(targetFd, ss.str());
}
