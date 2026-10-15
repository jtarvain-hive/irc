#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <cctype>

namespace {

// RFC 2812's "special": the two ASCII blocks just after A-Z and a-z
bool isSpecial(char ch)
{
    return (ch >= '[' && ch <= '`')     // [ \ ] ^ _ `
        || (ch >= '{' && ch <= '}');    // { | }
}

// RFC 2812: nickname = ( letter / special ) *8( letter / digit / special / "-" )
bool isValidNick(const std::string& n)
{
    if (n.empty() || n.size() > 9)
        return false;

    if (!std::isalpha(static_cast<unsigned char>(n[0])) && !isSpecial(n[0]))
        return false;

    for (size_t i = 1; i < n.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(n[i]))
            && !isSpecial(n[i]) && n[i] != '-')
            return false;
    }
    return true;
}

}   // namespace

void cmdNick(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty()) {
        server.sendToClient(fd, ":ircserv 431 " + c->replyNick() + " :No nickname given\r\n");
        return;
    }
    const std::string& nick = msg.params[0];

    if (!isValidNick(nick)) {
        server.sendToClient(fd, ":ircserv 432 " + c->replyNick() + " " + nick + " :Erroneous nickname\r\n");
        return;
    }

    if (server.isNickInUse(nick)) {
        server.sendToClient(fd, ":ircserv 433 " + c->replyNick() + " " + nick + " :Nickname is already in use\r\n");
        return;
    }

    std::string oldNick = c->getNick();
    if (!oldNick.empty())
        server.unregisterNick(oldNick);
    c->setNick(nick);
    server.registerNick(nick, fd);

    if (!c->isRegistered()) {
        server.tryRegister(fd);
        return;
    }

    // broadcast NICK change to channels the user is in (include self)
    std::stringstream ss;
    ss << ":" << oldNick << "!~" << c->getUser() << "@" << c->getIpAddr()
       << " NICK " << nick << "\r\n";
    server.sendToClient(fd, ss.str());
    server.broadcastToMemberChannels(fd, ss.str());
}
