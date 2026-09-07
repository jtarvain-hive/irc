#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

void cmdPrivmsg(Server& server, int fd, const Message& msg)
{
    Client* sender = server.getClient(fd);
    if (!sender) return;

    const std::string& target = msg.params[0];
    const std::string  text   = msg.trailing;

    // :sender_nick!sender_user@sender_host PRIVMSG <target> :<text>
    std::stringstream ss;
    ss << ":" << sender->getNick() << "!~" << sender->getUser()
       << "@" << sender->getIpAddr()
       << " PRIVMSG " << target << " :" << text << "\r\n";
    const std::string line = ss.str();

    // Channel target?
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '+' || target[0] == '!')) {
        Channel* channel = server.getChannel(target);
        if (!channel)
            return server.sendToClient(fd, ":ircserv 403 " + sender->getNick() + " " + target + " :No such channel\r\n");

        if (!server.isMember(target, sender->getNick()))
            return server.sendToClient(fd, ":ircserv 404 " + sender->getNick() + " " + target + " :Cannot send to channel\r\n");

        if (server.sendToChannel(target, line, fd))
            return;
        server.sendToClient(fd, ":ircserv 403 " + sender->getNick() + " " + target + " :No such channel\r\n");
        return;
    }

    // Nick target
    int targetFd = server.findFdByNick(target);
    if (targetFd < 0) {
        server.sendToClient(fd, ":ircserv 401 " + sender->getNick() + " " + target + " :No such nick/channel\r\n");
        return;
    }
    server.sendToClient(targetFd, line);
}
