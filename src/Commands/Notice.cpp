#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

void cmdNotice(Server& server, int fd, const Message& msg)
{
    Client* sender = server.getClient(fd);
    if (!sender || msg.params.empty() || msg.trailing.empty())
        return;

    const std::string& target = msg.params[0];

    std::stringstream ss;
    ss << ":" << sender->getNick() << "!~" << sender->getUser()
       << "@" << sender->getIpAddr()
       << " NOTICE " << target << " :" << msg.trailing << "\r\n";
    const std::string line = ss.str();

    if (target[0] == '#' || target[0] == '&') {
        server.sendToChannel(target, line, fd);
        return;
    }

    int targetFd = server.findFdByNick(target);
    if (targetFd >= 0)
        server.sendToClient(targetFd, line);
}
