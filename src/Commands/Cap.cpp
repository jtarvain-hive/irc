#include "Command.hpp"
#include "Server.hpp"

#include <cctype>

void cmdCap(Server& server, int fd, const Message& msg)
{
    if (msg.params.empty()) return;
    std::string sub = msg.params[0];
    for (char& ch : sub)
        ch = std::toupper(static_cast<unsigned char>(ch));

    if (sub == "LS")
        server.sendToClient(fd, ":ircserv CAP * LS :\r\n");
    else if (sub == "LIST")
        server.sendToClient(fd, ":ircserv CAP * LIST :\r\n");
    else if (sub == "REQ")
        server.sendToClient(fd, ":ircserv CAP * NAK :" + msg.trailing + "\r\n");
    // CAP END and anything else: no reply needed.
}
