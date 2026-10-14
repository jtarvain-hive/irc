#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

namespace {

std::vector<std::string> splitList(const std::string& s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (ch == sep) { out.push_back(cur); cur.clear(); }
        else cur += ch;
    }
    out.push_back(cur);
    return out;
}

void partOne(Server& server, int fd, const std::string& name, const std::string& reason)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    Channel* ch = server.getChannel(name);
    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + name + " :You're not on that channel\r\n");

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

}   // namespace

void cmdPart(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " PART :Not enough parameters\r\n");

    std::vector<std::string> names = splitList(msg.params[0], ',');

    for (size_t i = 0; i < names.size(); ++i) {
        if (names[i].empty()) continue;
        partOne(server, fd, names[i], msg.trailing);
    }
}
