#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <set>
#include <sstream>

namespace {

void privmsgOne(Server& server, int fd, const Client& sender,
                const std::string& target, const std::string& text)
{
    // :sender_nick!sender_user@sender_host PRIVMSG <target> :<text>
    std::stringstream ss;
    ss << ":" << sender.getNick() << "!~" << sender.getUser()
       << "@" << sender.getIpAddr()
       << " PRIVMSG " << target << " :" << text << "\r\n";
    const std::string line = ss.str();

    // Channel target?
    if (target[0] == '#' || target[0] == '&' || target[0] == '+' || target[0] == '!') {
        if (!server.getChannel(target))
            return server.sendToClient(fd, ":ircserv 403 " + sender.getNick() + " " + target + " :No such channel\r\n");

        if (!server.isMember(target, sender.getNick()))
            return server.sendToClient(fd, ":ircserv 404 " + sender.getNick() + " " + target + " :Cannot send to channel\r\n");

        server.sendToChannel(target, line, fd);
        return;
    }

    // Nick target
    int targetFd = server.findFdByNick(target);
    if (targetFd < 0)
        return server.sendToClient(fd, ":ircserv 401 " + sender.getNick() + " " + target + " :No such nick/channel\r\n");
    server.sendToClient(targetFd, line);
}

}   // namespace

void cmdPrivmsg(Server& server, int fd, const Message& msg)
{
    Client* sender = server.getClient(fd);
    if (!sender) return;

    std::string text;
    if (msg.hasTrailing)
        text = msg.trailing;
    else if (msg.params.size() >= 2)
        text = msg.params[1];

    if (text.empty())
        return server.sendToClient(fd, ":ircserv 412 " + sender->getNick() + " :No text to send\r\n");

    std::set<std::string, IrcLess> seen;
    for (const std::string& target : splitList(msg.params[0], ','))
        if (!target.empty() && seen.insert(target).second)
            privmsgOne(server, fd, *sender, target, text);
}
