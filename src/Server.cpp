#include "Server.hpp"
#include "Client.hpp"
#include <unistd.h>   // close()
#include <fcntl.h>    // fcntl(), F_SETFL, O_NONBLOCK
#include <cstring>    // std::memset
#include <stdexcept>  // std::runtime_error
#include <iostream>   // std::cout


void    Server::buildDispatch()
{
   dispatchCommand = {
        {"PASS", {&Server::handlePass, false, 1}},
        {"NICK", {&Server::handleNick, false, 1}},
        {"USER", {&Server::handleUser, false, 3}},
        {"PRIVMSG", {&Server::handlePrivMsg, true, 2}},
        {"PING", {&Server::handlePing, false, 1}},
        {"CAP", {&Server::handleCap, false, 1}}
   };
}

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

void    Server::disconnectClient(int fd)
{
    Client* c = getClient(fd);
    if (c && !c->getNick().empty()) {
        _nickToFd.erase(c->getNick());
    }

    close(fd);
    _clients.erase(fd);
    // Mark the pollfd for removal
    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd) { _pollFds[i].fd = -1; break; } 
}

Client* Server::getClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return nullptr;
    return &it->second;
}

void    Server::tryRegister(int fd)
{
    Client *c = getClient(fd);
    if (!c || c->isRegistered() || !c->gotPass() || !c->gotNick() || !c->gotUser())
    {
        std::stringstream ss;
        ss << ":ircserv NOTICE * :Registration status: PASS(" << (c->gotPass() ? c->getPass() : "NO")
           << ") NICK(" << (c->gotNick() ? c->getNick() : "NO")
           << ") USER(" << (c->gotUser() ? c->getUser() : "NO") << ")\r\n";

        sendToClient(fd, ss.str());
        return;
    }

    if (c->getPass() != _password)
    {
        sendToClient(fd, ":ircserv 464 * :Password incorrect\r\n");
        flushToClient(fd);
        disconnectClient(fd);
        return;
    }

    c->setRegistered(true);
    // RPL_WELCOME
    sendToClient(fd, ":ircserv 001 " + c->getNick() + " :Welcome to the Internet Relay Network " + c->getNick() + "\r\n");
    // You would typically send more welcome messages here (002, 003, 004, etc.)
}

Message Server::parseMessage(const std::string& line) {
    Message msg;
    std::string remaining = line;

    // 1. Remove leading spaces
    remaining.erase(0, remaining.find_first_not_of(" "));
    if (remaining.empty()) return msg;

    // 2. Ignore prefix (starts with ':') for client messages
    if (remaining[0] == ':') {
        size_t spacePos = remaining.find(' ');
        if (spacePos == std::string::npos) return msg;
        remaining.erase(0, spacePos + 1);
        remaining.erase(0, remaining.find_first_not_of(" "));
    }

    // 3. Extract Command
    size_t spacePos = remaining.find(' ');
    if (spacePos != std::string::npos) {
        msg.cmd = remaining.substr(0, spacePos);
        remaining.erase(0, spacePos + 1);
    } else {
        msg.cmd = remaining;
        remaining.clear();
    }

    // Capitalize command
    for (char& ch : msg.cmd) {
        ch = std::toupper(static_cast<unsigned char>(ch));
    }

    // 4. Extract Parameters
    while (!remaining.empty()) {
        remaining.erase(0, remaining.find_first_not_of(" "));
        if (remaining.empty()) break;

        // If we hit a ':', the rest of the string is the trailing parameter
        if (remaining[0] == ':') {
            msg.trailing = remaining.substr(1);
            break;
        }

        spacePos = remaining.find(' ');
        if (spacePos != std::string::npos) {
            msg.params.push_back(remaining.substr(0, spacePos));
            remaining.erase(0, spacePos + 1);
        } else {
            msg.params.push_back(remaining);
            remaining.clear();
        }
    }

    return msg;
}

void    Server::handleCommand(int fd, const std::string& line)
{
    Client* cp = getClient(fd);
    if (!cp) return;
    Client& c = *cp;

    Message msg = parseMessage(line);
    if (msg.cmd.empty()) return;

    if (auto it = dispatchCommand.find(msg.cmd); it != dispatchCommand.end()) {
        const Command& cmdDef = it->second;

        if (cmdDef.needsRegistration && !c.isRegistered())
            return sendToClient(fd, ":ircserv 451 * :You have not registered\r\n");
        
        // The trailing part is also a parameter
        if (msg.params.size() < cmdDef.minParams)
            return sendToClient(fd, ":ircserv 461 " + c.getNick() + " " + msg.cmd + " :Not enough parameters\r\n");

        std::invoke(cmdDef.fn, this, fd, msg);
    } else {
        sendToClient(fd, ":ircserv 421 * " + msg.cmd + " :Unknown command\r\n");
    }

}

void    Server::readFromClient(int fd)
{
    char    tmp[512];
    ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

    if (n == 0)
        return disconnectClient(fd);
    if (n < 0)
        return ;

    Client* cp = getClient(fd);
    if (!cp)
        return;
    Client& c = *cp;
    c.recvBuf().append(tmp, n);

    size_t pos;
    while ((pos = c.recvBuf().find('\n')) != std::string::npos)
    {
        std::string line = c.recvBuf().substr(0, pos);
        c.recvBuf().erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
            handleCommand(fd, line);
    }
}

void    Server::sendToClient(int fd, const std::string& msg)
{
    Client* c = getClient(fd);
    if (!c)
        return;
    c->outBuf() += msg;
    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd) { _pollFds[i].events |= POLLOUT; break; }
}

void    Server::flushToClient(int fd)
{
    Client* cp = getClient(fd);
    if (!cp)
        return;
    Client& c = *cp;
    if (c.outBuf().empty())
        return;

    ssize_t n = send(fd, c.outBuf().data(), c.outBuf().size(), 0);
    if (n <= 0)
        return ;
    c.outBuf().erase(0, n);
    if (c.outBuf().empty())
        for (size_t i = 0; i < _pollFds.size(); ++i)
            if (_pollFds[i].fd == fd)  { _pollFds[i].events &= ~POLLOUT; break; }
}

void    Server::run() {
    while (!_signalReceived) {
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);

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
                {
                    readFromClient(_pollFds[i].fd);
                    // std::cout << "read from client" << std::endl;
                }
            }
            if (_pollFds[i].revents & POLLOUT)
            {
                flushToClient(_pollFds[i].fd);
                // std::cout << "flush to client" << std::endl;
            }
        }
        _pollFds.erase(std::remove_if(_pollFds.begin(), _pollFds.end(),
            [](const pollfd& p){ return p.fd == -1; }), _pollFds.end());
    }
}