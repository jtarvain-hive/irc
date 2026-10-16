#include "Server.hpp"
#include "Client.hpp"

#include <unistd.h>   // close()
#include <fcntl.h>    // fcntl(), F_SETFL, O_NONBLOCK
#include <cstring>    // std::memset
#include <stdexcept>  // std::runtime_error
#include <iostream>   // std::cout
#include <algorithm>  // std::remove_if
#include <cerrno>     // errno, EINTR

Server::Server(int port, const std::string& password)
    : _listenFd(-1), _port(port), _password(password)
{
    buildDispatch();
}

Server::~Server()
{
    if (_listenFd != -1)
        close(_listenFd);
}

volatile sig_atomic_t Server::_signalReceived = false;

void Server::signalHandler(int signum) {
    (void)signum;
    _signalReceived = true;
}

void Server::init()
{
    signal(SIGINT,  Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);
    signal(SIGPIPE, SIG_IGN);

    this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_listenFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");

    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl() failed");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(_port);

    if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    struct pollfd pfd;
    pfd.fd     = _listenFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);

    std::cout << "Server is listening" << std::endl;
}

void Server::acceptNewClient()
{
    struct sockaddr_in  clientAddr;
    socklen_t           len = sizeof(clientAddr);

    int clientFd = accept(_listenFd, (struct sockaddr *)&clientAddr, &len);
    if (clientFd < 0)
        return;
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
    pfd.fd     = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
}

// Everything about leaving except closing the socket: QUIT to channel peers,
// leave channels, free the nick.
void Server::detachClient(int fd, const std::string& reason)
{
    Client* c = getClient(fd);
    if (c) {
        // broadcast before removeClientFromChannels: the fd must still be a
        // member for broadcastToMemberChannels to find its channels
        std::string who = c->getNick().empty() ? std::string("*") : c->getNick();
        broadcastToMemberChannels(fd, ":" + who + "!~" + c->getUser() + "@"
                                      + c->getIpAddr() + " QUIT :" + reason + "\r\n");
        if (findFdByNick(c->getNick()) == fd)
            _nickToFd.erase(c->getNick());
    }

    removeClientFromChannels(fd);
}

void Server::disconnectClient(int fd, const std::string& reason)
{
    detachClient(fd, reason);
    _clients.erase(fd);
    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd) { _pollFds[i].fd = -1; break; }
}

// The socket is closed by flushToClient once ERROR has been sent.
void Server::closeAfterFlush(int fd, const std::string& reason)
{
    Client* c = getClient(fd);
    if (!c || c->isClosing()) return;

    detachClient(fd, reason);
    sendToClient(fd, "ERROR :Closing link (" + reason + ")\r\n");
    c->setClosing();
    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd) { _pollFds[i].events = POLLOUT; break; }
}

void Server::readFromClient(int fd)
{
    char    tmp[512];
    ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

    if (n <= 0)
        return disconnectClient(fd);

    Client* c = getClient(fd);
    if (!c) return;
    c->recvBuf().append(tmp, n);

    size_t pos;
    while ((pos = c->recvBuf().find('\n')) != std::string::npos)
    {
        std::string line = c->recvBuf().substr(0, pos);
        c->recvBuf().erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
        {
            handleCommand(fd, line);
            c = getClient(fd);   // handleCommand may have disconnected this client
            if (!c || c->isClosing()) return;
        }
    }
}

void Server::sendToClient(int fd, const std::string& msg)
{
    Client* c = getClient(fd);
    if (!c) return;
    c->outBuf() += msg;
    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd) { _pollFds[i].events |= POLLOUT; break; }
}

void Server::flushToClient(int fd)
{
    Client* cp = getClient(fd);
    if (!cp) return;
    Client& c = *cp;
    if (c.outBuf().empty()) return;

    ssize_t n = send(fd, c.outBuf().data(), c.outBuf().size(), 0);
    if (n < 0)
        return disconnectClient(fd);
    c.outBuf().erase(0, n);
    if (c.outBuf().empty() && c.isClosing())
        return disconnectClient(fd);
    if (c.outBuf().empty())
        for (size_t i = 0; i < _pollFds.size(); ++i)
            if (_pollFds[i].fd == fd) { _pollFds[i].events &= ~POLLOUT; break; }
}

void Server::run() {
    while (!_signalReceived) {
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);

        if (ready < 0) {
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
            else if (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                if (_pollFds[i].fd != _listenFd)
                    disconnectClient(_pollFds[i].fd);
            }
            if (_pollFds[i].revents & POLLOUT)
                flushToClient(_pollFds[i].fd);
        }
        _pollFds.erase(std::remove_if(_pollFds.begin(), _pollFds.end(),
            [](const pollfd& p){ return p.fd == -1; }), _pollFds.end());
    }
}