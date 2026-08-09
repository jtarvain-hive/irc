#include "Command.hpp"
#include "../../include/Server.hpp"
#include "../../include/Client.hpp"

void cmdTopic(Server& server, int fd, const Message& msg)
{
    Client* c = server.getClient(fd);
    if (!c) return;

    if (msg.params.empty())
        return server.sendToClient(fd, ":ircserv 461 " + c->getNick() + " TOPIC :Not enough parameters\r\n");

    const std::string& channel = msg.params[0];

    // No trailing -> query the topic
    Channel* ch = server.getChannel(channel);
    if (msg.trailing.empty()) {
        if (!ch || ch->getTopic().empty())
            return server.sendToClient(fd, ":ircserv 331 " + c->getNick() + " " + channel + " :No topic is set\r\n");
        return server.sendToClient(fd, ":ircserv 332 " + c->getNick() + " " + channel + " :" + ch->getTopic() + "\r\n");
    }

    // Setting the topic
    if (!ch || !ch->hasMember(fd))
        return server.sendToClient(fd, ":ircserv 442 " + c->getNick() + " " + channel + " :You're not on that channel\r\n");

    if (ch->isTopicRestricted() && !ch->isOperator(fd))
        return server.sendToClient(fd, ":ircserv 482 " + c->getNick() + " " + channel + " :You're not a channel operator\r\n");

    ch->setTopic(msg.trailing);
    std::stringstream ss;
    ss << ":" << c->getNick() << "!~" << c->getUser()
       << "@" << c->getIpAddr()
       << " TOPIC " << channel << " :" << msg.trailing << "\r\n";
    server.sendToChannel(channel, ss.str());
}
