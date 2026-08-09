#include "Server.hpp"
#include "Command.hpp"

#include <cctype>

// Forward-declared command handlers (defined in src/Commands/*.cpp).
void cmdPass  (Server&, int, const Message&);
void cmdNick  (Server&, int, const Message&);
void cmdUser  (Server&, int, const Message&);
void cmdPrivmsg(Server&, int, const Message&);
void cmdPing  (Server&, int, const Message&);
void cmdCap   (Server&, int, const Message&);
void cmdQuit  (Server&, int, const Message&);
void cmdJoin  (Server&, int, const Message&);
void cmdPart  (Server&, int, const Message&);
void cmdKick  (Server&, int, const Message&);
void cmdInvite(Server&, int, const Message&);
void cmdTopic (Server&, int, const Message&);
void cmdMode  (Server&, int, const Message&);

void Server::buildDispatch()
{
    dispatchCommand = {
        {"PASS",   { cmdPass,   false, 1 }},
        {"NICK",   { cmdNick,   false, 1 }},
        {"USER",   { cmdUser,   false, 3 }},
        {"PRIVMSG",{ cmdPrivmsg,true,  2 }},
        {"PING",   { cmdPing,   false, 1 }},
        {"CAP",    { cmdCap,    false, 1 }},
        {"QUIT",   { cmdQuit,   false, 0 }},
        {"JOIN",   { cmdJoin,   true,  1 }},
        {"PART",   { cmdPart,   true,  1 }},
        {"KICK",   { cmdKick,   true,  2 }},
        {"INVITE", { cmdInvite, true,  2 }},
        {"TOPIC",  { cmdTopic,  true,  1 }},
        {"MODE",   { cmdMode,   true,  1 }},
    };
}

Message Server::parseMessage(const std::string& line) {
    Message msg;
    std::string remaining = line;

    remaining.erase(0, remaining.find_first_not_of(" "));
    if (remaining.empty()) return msg;

    if (remaining[0] == ':') {
        size_t spacePos = remaining.find(' ');
        if (spacePos == std::string::npos) return msg;
        remaining.erase(0, spacePos + 1);
        remaining.erase(0, remaining.find_first_not_of(" "));
    }

    size_t spacePos = remaining.find(' ');
    if (spacePos != std::string::npos) {
        msg.cmd = remaining.substr(0, spacePos);
        remaining.erase(0, spacePos + 1);
    } else {
        msg.cmd = remaining;
        remaining.clear();
    }

    for (char& ch : msg.cmd)
        ch = std::toupper(static_cast<unsigned char>(ch));

    while (!remaining.empty()) {
        remaining.erase(0, remaining.find_first_not_of(" "));
        if (remaining.empty()) break;

        if (remaining[0] == ':') {
            msg.trailing = remaining.substr(1);
            break;
        }

        spacePos = remaining.find(' ');
        if (spacePos != std::string::npos) {
            msg.params.push_back(remaining.substr(0, spacePos));
            remaining.erase(0, spacePos + 1);
        } else {
            msg.params.push_back(remaining);
            remaining.clear();
        }
    }

    return msg;
}

void Server::handleCommand(int fd, const std::string& line)
{
    Client* cp = getClient(fd);
    if (!cp) return;
    const Client& c = *cp;

    Message msg = parseMessage(line);
    if (msg.cmd.empty()) return;

    auto it = dispatchCommand.find(msg.cmd);
    if (it == dispatchCommand.end()) {
        sendToClient(fd, ":ircserv 421 " + c.getNick() + " " + msg.cmd + " :Unknown command\r\n");
        return;
    }

    const Command& def = it->second;

    if (def.needsRegistration && !c.isRegistered()) {
        sendToClient(fd, ":ircserv 451 " + c.getNick() + " :You have not registered\r\n");
        return;
    }

    // trailing counts as a parameter when present
    size_t provided = msg.params.size() + (msg.trailing.empty() ? 0 : 1);
    if (provided < def.minParams) {
        sendToClient(fd, ":ircserv 461 " + c.getNick() + " " + msg.cmd + " :Not enough parameters\r\n");
        return;
    }

    def.fn(*this, fd, msg);
}

void Server::tryRegister(int fd)
{
    Client* c = getClient(fd);
    if (!c) return;

    if (c->isRegistered())
        return;

    if (!c->gotPass() || !c->gotNick() || !c->gotUser())
    {
        // Silent on intermediate steps; uncomment for debugging.
        // std::stringstream ss;
        // ss << ":ircserv NOTICE * :Registration status: PASS(" << ...
        // sendToClient(fd, ss.str());
        return;
    }

    if (c->getPass() != _password)
    {
        sendToClient(fd, ":ircserv 464 " + c->getNick() + " :Password incorrect\r\n");
        flushToClient(fd);
        disconnectClient(fd);
        return;
    }

    c->setRegistered(true);
    sendToClient(fd, ":ircserv 001 " + c->getNick() + " :Welcome to the Internet Relay Network " + c->getNick() + "\r\n");
    // 002..004 numerics would go here.
}