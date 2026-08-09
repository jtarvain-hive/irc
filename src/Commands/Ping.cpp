#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

void cmdPing(Server& server, int fd, const Message& msg)
{
    (void)server.getClient(fd); // existence check is implicit; PING works pre-registration
    server.sendToClient(fd, ":ircserv PONG ircserv :" + msg.params[0] + "\r\n");
}
