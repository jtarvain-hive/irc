#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>
#include <cstdlib>

namespace {

// returns true on '+k' / '+l' to signal that an extra parameter is required
bool applyMode(Server& server, int fd,
               const std::string& channel, char mode, char sign,
               const std::string& arg, std::string& outReply)
{
    Client* c = server.getClient(fd);
    (void)c;

    switch (mode) {
    case 'i':
        server.setChannelInviteOnly(channel, sign == '+');
        outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " " + sign + "i\r\n";
        return true;
    case 't':
        server.setChannelTopicRestricted(channel, sign == '+');
        outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " " + sign + "t\r\n";
        return true;
    case 'o':
        // arg = target nick; toggle operator on that nick
        server.setChannelOperator(channel, arg, sign == '+');
        outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " " + sign + "o " + arg + "\r\n";
        return true;
    case 'k':
        // arg = new key (only meaningful on '+')
        if (sign == '+') server.setChannelKey(channel, arg);
        else server.clearChannelKey(channel);
        outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " " + sign + "k" + (sign == '+' ? " " + arg : "") + "\r\n";
        return true;
    case 'l':
        if (sign == '+') {
            int limit = std::atoi(arg.c_str());
            server.setChannelUserLimit(channel, limit);
            outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " +l " + arg + "\r\n";
        } else {
            server.clearChannelUserLimit(channel);
            outReply = ":" + (c ? c->getNick() : std::string("ircserv")) + " MODE " + channel + " -l\r\n";
        }
        return true;
    default:
        outReply = ":ircserv 472 " + (c ? c->getNick() : std::string("*")) + " ";
        outReply.push_back(mode);
        outReply += " :is unknown mode char to me\r\n";
        return false;
    }
}

} // namespace

void cmdMode(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " MODE :Not enough parameters\r\n");

    const std::string& channel = msg.params[0];
    Channel* ch = server.getChannel(channel);

    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + channel + " :You're not on that channel\r\n");

    // "MODE #chan" with no further args -> RPL_CHANNELMODEIS (324)
    if (msg.params.size() == 1) {
        server.replyChannelMode(fd, channel);
        return;
    }

    // "MODE #chan b" -> list bans (not required by subject, but clients send it)
    if (msg.params.size() == 2 && msg.params[1] == "b") {
        server.sendToClient(fd, ":ircserv 368 " + c->getNick() + " " + channel + " :End of channel ban list\r\n");
        return;
    }

    if (!ch->isOperator(fd))
        return server.sendToClient(fd, ":ircserv 482 " + c->getNick() + " " + channel + " :You're not a channel operator\r\n");

    const std::string& modestring = msg.params[1];

    // walk the modestring, building a sign + mode letter per change.
    // extra-arg modes: o, k, l  (consume from msg.params[2..])
    char   sign = '+';
    size_t argIdx = 2;
    for (size_t i = 0; i < modestring.size(); ++i) {
        char ch = modestring[i];
        if (ch == '+' || ch == '-') { sign = ch; continue; }

        std::string arg;
        bool needsArg = (ch == 'o' || ch == 'k' || ch == 'l');
        if (needsArg) {
            if (argIdx >= msg.params.size()) {
                server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " MODE :Not enough parameters\r\n");
                return;
            }
            arg = msg.params[argIdx++];
        }

        std::string reply;
        bool changed = applyMode(server, fd, channel, ch, sign, arg, reply);
        server.sendToClient(fd, reply);
        if (changed)
            server.sendToChannel(channel, reply, fd);
    }
}
