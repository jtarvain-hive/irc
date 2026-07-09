#pragma once

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <exception>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <csignal>

class Client;
class Channel;

class Server {
public:
    Server() = delete;
    Server(int port, const std::string& password);
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
    std::map<std::string, Channel>  _channels;


    void    acceptNewClient();
    void    readFromClient(int fd);
    void    flushToClient(int fd);

    void    closeFds();
    void    clearClients(int fd);

    static volatile sig_atomic_t _signalReceived;
    static void signalHandler(int signum);

};