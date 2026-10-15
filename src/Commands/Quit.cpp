#include "Command.hpp"
#include "Server.hpp"

void cmdQuit(Server& server, int fd, const Message& msg)
{
    std::string reason = msg.trailing;
    if (reason.empty())
        reason = "Client Quit";

    // broadcasts the QUIT now, closes the socket once ERROR is flushed
    server.closeAfterFlush(fd, "Quit: " + reason);
}
