#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

void cmdQuit(Server& server, int fd, const Message& msg)
{
    std::string reason = msg.trailing;
    if (reason.empty())
        reason = "Client Quit";

    // disconnectClient broadcasts the QUIT to every channel this client is in
    server.disconnectClient(fd, "Quit: " + reason);
}