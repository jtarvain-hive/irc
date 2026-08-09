#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"



void cmdJoin(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " JOIN :Not enough parameters\r\n");

    // Spec allows comma-separated channel list: "JOIN #a,#b key1,"
    // For now we support a single channel.
    const std::string& name = msg.params[0];
    if (name.empty() || (name[0] != '#' && name[0] != '&'))
        return server.sendToClient(fd, ":ircserv 403 " + c->getNick() + " " + name + " :No such channel\r\n");

    const std::string key = (msg.params.size() >= 2) ? msg.params[1] : std::string();

    Channel* ch = server.getChannel(name);
    if (ch && ch->isInviteOnly() && !ch->isInvited(fd))
        return server.sendToClient(fd, ":ircserv 473 " + c->getNick() + " " + name + " :Cannot join channel (invite only)\r\n");

    if (ch && ch->hasKey() && ch->getKey() != key)
        return server.sendToClient(fd, ":ircserv 475 " + c->getNick() + " " + name + " :Bad channel key\r\n");

    if (ch && ch->hasUserLimit() && ch->getMembers().size() >= ch->getUserLimit())
        return server.sendToClient(fd, ":ircserv 471 " + c->getNick() + " " + name + " :Channel is full\r\n");

    if (!server.addMemberToChannel(name, fd, key))
        return server.sendToClient(fd, ":ircserv 403 " + c->getNick() + " " + name + " :No such channel\r\n");

    ch = server.getChannel(name);
    if (!ch)
        return;

    // First member becomes operator
    if (ch->getMembers().size() == 1)
        ch->addOperator(fd);

    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " JOIN " << name << "\r\n";
    server.sendToChannel(name, ss.str());

    // Topic
    if (ch->getTopic().empty())
        server.sendToClient(fd, ":ircserv 331 " + c->getNick() + " " + name + " :No topic is set\r\n");
    else
        server.sendToClient(fd, ":ircserv 332 " + c->getNick() + " " + name + " :" + ch->getTopic() + "\r\n");

    // NAMES reply
    std::string names;
    for (std::set<int>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it) {
        Client* m = server.getClient(*it);
        if (!m) continue;
        if (!names.empty()) names += " ";
        if (ch->isOperator(*it)) names += "@" + m->getNick(); else names += m->getNick();
    }
    server.sendToClient(fd, ":ircserv 353 " + c->getNick() + " = " + name + " :" + names + "\r\n");
    server.sendToClient(fd, ":ircserv 366 " + c->getNick() + " " + name + " :End of NAMES list\r\n");
}

 
