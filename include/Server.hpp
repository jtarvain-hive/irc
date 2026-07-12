#pragma once

#include <algorithm>

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <exception>

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <csignal>

#include <functional>

class Client;
class Channel;
class Server;

struct Message {
    std::string                 cmd;
    std::vector<std::string>    params;
    std::string                 trailing;
};

using Handler = void (Server::*)(int, const Message&);

struct Command {
    Handler fn;
    bool    needsRegistration;
    size_t  minParams;
};


class Server {
public:
    Server(int port, const std::string& password);


    Server() = delete;
    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;
    ~Server();

    void    init();
    void    run();
    
private:
    int         _listenFd;    
    int         _port;
    std::string _password;

    std::vector<struct pollfd>      _pollFds;
    std::map<int, Client>           _clients;

    std::unordered_map<std::string, int> _nickToFd;
    // std::map<std::string, Channel>  _channels;  // TODO: enable once Channel is implemented

    std::unordered_map<std::string_view, Command> dispatchCommand;

    void    buildDispatch();

    Client* getClient(int fd);      // returns nullptr if the fd is not a known client
    void    acceptNewClient();
    void    sendToClient(int fd, const std::string& line);
    void    disconnectClient(int fd);

    void    readFromClient(int fd); // recv() from client
    void    flushToClient(int fd); // send() to client

    void    closeFds();
    void    clearClients(int fd);

    static volatile sig_atomic_t _signalReceived;
    static void signalHandler(int signum);

    Message parseMessage(const std::string& line);

    void    handleCommand(int fd, const std::string& line);
    void    handlePass(int fd, const Message& msg);
    void    handleNick(int fd, const Message& msg);
    void    handleUser(int fd, const Message& msg);
    void    handlePrivMsg(int fd, const Message& msg);
    void    handlePing(int fd, const Message& msg);
    void    tryRegister(int fd);
    
};