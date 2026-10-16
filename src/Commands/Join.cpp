#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"



namespace {

void joinOne(Server& server, int fd, const std::string& name, const std::string& key)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (name.empty() || (name[0] != '#' && name[0] != '&'))
        return server.sendToClient(fd, ":ircserv 403 " + c->getNick() + " " + name + " :No such channel\r\n");

    Channel* ch = server.getChannel(name);
    // Rejoining an existing membership must not recheck access or emit JOIN.
    if (ch && ch->hasMember(fd))
        return;

    if (ch && ch->isInviteOnly() && !ch->isInvited(fd))
        return server.sendToClient(fd, ":ircserv 473 " + c->getNick() + " " + name + " :Cannot join channel (invite only)\r\n");

    if (ch && ch->hasKey() && ch->getKey() != key)
        return server.sendToClient(fd, ":ircserv 475 " + c->getNick() + " " + name + " :Bad channel key\r\n");

    if (ch && ch->hasUserLimit() && ch->getMembers().size() >= ch->getUserLimit())
        return server.sendToClient(fd, ":ircserv 471 " + c->getNick() + " " + name + " :Channel is full\r\n");

    server.addMemberToChannel(name, fd);

    ch = server.getChannel(name);
    if (!ch)
        return;

    // replies use the channel's stored spelling, not the case the client
    // typed, so every member sees the same channel name
    const std::string& cname = ch->getName();

    // First member becomes operator
    if (ch->getMembers().size() == 1)
        ch->addOperator(fd);

    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " JOIN " << cname << "\r\n";
    server.sendToChannel(cname, ss.str());

    // Topic
    if (ch->getTopic().empty())
        server.sendToClient(fd, ":ircserv 331 " + c->getNick() + " " + cname + " :No topic is set\r\n");
    else
        server.sendToClient(fd, ":ircserv 332 " + c->getNick() + " " + cname + " :" + ch->getTopic() + "\r\n");

    // NAMES reply
    std::string names;
    for (std::set<int>::const_iterator it = ch->getMembers().begin(); it != ch->getMembers().end(); ++it) {
        Client* m = server.getClient(*it);
        if (!m) continue;
        if (!names.empty()) names += " ";
        if (ch->isOperator(*it)) names += "@" + m->getNick(); else names += m->getNick();
    }
    server.sendToClient(fd, ":ircserv 353 " + c->getNick() + " = " + cname + " :" + names + "\r\n");
    server.sendToClient(fd, ":ircserv 366 " + c->getNick() + " " + cname + " :End of NAMES list\r\n");
}

}   // namespace

void cmdJoin(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " JOIN :Not enough parameters\r\n");

    std::vector<std::string> names = splitList(msg.params[0], ',');
    std::vector<std::string> keys  = (msg.params.size() >= 2)
                                   ? splitList(msg.params[1], ',')
                                   : std::vector<std::string>();

    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i].empty()) continue;
        joinOne(server, fd, names[i], (i < keys.size()) ? keys[i] : std::string());
    }
}

 
