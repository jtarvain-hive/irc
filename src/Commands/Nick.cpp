#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdNick(Server& server, int fd, const Message& msg)
{
    if (msg.params.empty()) {
        server.sendToClient(fd, ":ircserv 431 * :No nickname given\r\n");
        return;
    }
    const std::string& nick = msg.params[0];

    if (server.isNickInUse(nick)) {
        server.sendToClient(fd, ":ircserv 433 * " + nick + " :Nickname is already in use\r\n");
        return;
    }

    Client* c = server.getClient(fd);
    if (!c) return;

    std::string oldNick = c->getNick();
    if (c->isRegistered() && !oldNick.empty()) {
        // Already registered -> update the nick mapping
        server.unregisterNick(oldNick);
        c->setNick(nick);
        server.registerNick(nick, fd);
    } else {
        c->setNick(nick);
        server.registerNick(nick, fd);
    }

    if (oldNick.empty()) {
        server.tryRegister(fd);
        return;
    }

    // broadcast NICK change to channels the user is in (include self)
    std::stringstream ss;
    ss << ":" << oldNick << "!~" << c->getUser() << "@" << c->getIpAddr()
       << " NICK " << nick << "\r\n";
    server.sendToClient(fd, ss.str());
    server.broadcastToMemberChannels(fd, ss.str());
    server.tryRegister(fd);
}
