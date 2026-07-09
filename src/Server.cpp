#include "Server.hpp"
#include "Client.hpp"
#include <unistd.h>   // close()
#include <fcntl.h>    // fcntl(), F_SETFL, O_NONBLOCK
#include <cstring>    // std::memset
#include <stdexcept>  // std::runtime_error
#include <iostream>   // std::cout

Server::Server(int port, const std::string& password)
    : _listenFd(-1), _port(port), _password(password)
{

}

Server::~Server()
{
    if (_listenFd != -1)
        close(_listenFd);
}

volatile sig_atomic_t    Server::_signalReceived = false;

void    Server::signalHandler(int signum) {
    (void)signum;
    _signalReceived = true;
}

void    Server::init()
{
    signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);
    // creates the endpoint, return an fd (or -1)
    // AF_INET = address family IPv4.. no need for IPv6
    // SOCK_STREAM = TCP stream
    // 0 = default protocol -> TCP (no getprotoby name)

    this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);

    if (this->_listenFd < 0)
        throw std::runtime_error("socket() failed");

    // SOL_SOCKET = "this option lives at the socket level" (not TCP or IP level)
    // SO_REUSEADDR = with opt = 1, it lets you re-bind a port that s still in TIME_WAIT. when a server closes, the OS
    // keeps the port reserved for a minute or two. Without this, restarting the server immediately gives bind() failed;
    int opt = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");
    
    // fcntl = file control
    // F_SETFL = the command, set the file status flags
    // O_NONBLOCK = non-blocking mode. accept() on this socket returns immedieatly with -1/EAGAIN when no connection is waiting instead
    // of blocking the whole process.  
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl() failed");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    // bind attaches the {IP, port} in addr to the socket. sockaddr_in is for ipv4 we cast to the generic base sockaddr for bind.. but it s ipv4
    // if it was sockaddr_in6 (IPv6) we would cast to the same base and it would work the same..
    // sin_family = AF_INET (IPv4)
    // s_addr = INADDR_ANY // 0.0.0.0 = accept on ALL local interfaces (loopback, LAN, etc..) not any specific ip but all of them i guess
    // sin_port = htons(_port) port converted to network byte order
    if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    // turns the socket passive (accept-only) and sets the backlog.
    // SOMAXCONN = the system s max allowed backlog (128) - the queue length for connections that have arrived but we haven t accept()ed yet
    if (listen(_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");
    
    struct pollfd pfd;

    pfd.fd = _listenFd; // watch the listen socket
    pfd.events = POLLIN; // for "readable" = a new connection is knocking
    pfd.revents = 0;     // kernel fills this in; start it clean
    _pollFds.push_back(pfd);
    std::cout << "Server is listening" << std::endl;
}


void    Server::acceptNewClient()
{
    struct sockaddr_in  clientAddr;
    socklen_t           len = sizeof(clientAddr);

    int clientFd = accept(_listenFd, (struct sockaddr *)&clientAddr, &len);
    if (clientFd < 0)
        return ;
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    std::string ip = inet_ntoa(clientAddr.sin_addr);
    int         clientPort = ntohs(clientAddr.sin_port);
    _clients.try_emplace(clientFd, clientFd, ip);

    std::cout << "[+] New client connected"
              << " | fd=" << clientFd
              << " | ip=" << ip
              << " | port=" << clientPort
              << " | total clients=" << _clients.size()
              << std::endl;

    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
}

void    Server::readFromClient(int fd)
{
    (void)fd;
}

void    Server::flushToClient(int fd)
{
    (void)fd;
}

void    Server::run() {
    while (!_signalReceived) {
        // _listenFd
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);
        // new client wants to connect
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("poll() failed");
        }

        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents & POLLIN) {
                if (_pollFds[i].fd == _listenFd)
                    acceptNewClient();
                else
                    readFromClient(_pollFds[i].fd);
            }
            if (_pollFds[i].revents & POLLOUT)
                flushToClient(_pollFds[i].fd);
        }
    }
}