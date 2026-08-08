#include "Server.hpp"
#include "Client.hpp"

void    Server::handlePass(int fd, const Message& msg)
{
    Client *c = getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return sendToClient(fd, ":ircserv 462 * :You may not reregister\r\n");

    if (msg.params.empty())
        return sendToClient(fd, ":ircserv 461 * PASS :Not enough parameters\r\n");

    c->setPass(msg.params[0]);
    sendToClient(fd, ":ircserv NOTICE * :PASS command received\r\n");
    tryRegister(fd);
}

void    Server::handleUser(int fd, const Message& msg)
{
    Client* c = getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return sendToClient(fd, ":ircserv 462 * :You may not reregister\r\n");

    // We already check for minParams in handleCommand, so this is safe.
    // Parameters are: <username> <hostname> <servername> <realname>
    // We only care about the username and realname for now.
    std::string user = msg.params[0];
    std::string realname = msg.trailing;

    // The realname can be the trailing parameter or the 4th parameter if no trailing is present.
    if (msg.trailing.empty() && msg.params.size() >= 4)
        c->setUser(user, msg.params[3]);
    else
        c->setUser(user, realname);

    tryRegister(fd);
}

void    Server::handleNick(int fd, const Message& msg)
{
    if (msg.params.empty()) {
        sendToClient(fd, ":ircserv 431 * :No nickname given\r\n");
        return;
    }
    std::string nick = msg.params[0];

    // Check if nick is already in use
    if (_nickToFd.count(nick)) {
        sendToClient(fd, ":ircserv 433 * " + nick + " :Nickname is already in use\r\n");
        return;
    }

    Client* c = getClient(fd);
    if (!c) return;

    if (c->isRegistered() && !c->getNick().empty()) {
        // If already registered, update the nick mapping
        _nickToFd.erase(c->getNick());
        c->setNick(nick);
        _nickToFd[nick] = fd;
        // TODO: Send NICK change notification to other clients
    } else {
        c->setNick(nick);
        _nickToFd[nick] = fd;
    }
    tryRegister(fd);
}

void    Server::handlePrivMsg(int fd, const Message& msg)
{
    std::string targetNick = msg.params[0];
    std::string text = msg.trailing;

    Client* sender = getClient(fd);
    if (!sender) return;

    // Find the target client's fd
    auto it = _nickToFd.find(targetNick);
    if (it == _nickToFd.end()) {
        sendToClient(fd, ":ircserv 401 " + sender->getNick() + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }
    int targetFd = it->second;

    // Format the message correctly: :<sender_nick>!<sender_user>@<sender_host> PRIVMSG <target_nick> :<text>
    std::stringstream ss;
    ss << ":" << sender->getNick() << " PRIVMSG " << targetNick << " :" << text << "\r\n";

    sendToClient(targetFd, ss.str());
}

void    Server::handlePing(int fd, const Message& msg)
{
    Client* c = getClient(fd);
    if (!c) return;
    sendToClient(fd, ":ircserv PONG ircserv :" + msg.params[0] + "\r\n");
}

void    Server::handleCap(int fd, const Message& msg)
{
    std::string sub = msg.params[0];
    for (char& ch : sub)
        ch = std::toupper(static_cast<unsigned char>(ch));

    if (sub == "LS")
        sendToClient(fd, ":ircserv CAP * LS :\r\n");
    else if (sub == "LIST")
        sendToClient(fd, ":ircserv CAP * LIST :\r\n");
    else if (sub == "REQ")
        sendToClient(fd, ":ircserv CAP * NAK :" + msg.trailing + "\r\n");
    // CAP END and anything else: no reply needed, client just proceeds.
}
