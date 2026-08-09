#pragma once

#include <algorithm>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <string_view>
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

#include "Client.hpp"
#include "Channel.hpp"
#include "Command.hpp"

struct Message {
    std::string                 cmd;
    std::vector<std::string>    params;
    std::string                 trailing;
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

    // --- accessors used by command handlers (defined in src/Commands/*) ---

    Client* getClient(int fd);                              // nullptr if fd is not a known client
    int     findFdByNick(const std::string& nick) const;    // -1 if not found
    bool    isNickInUse(const std::string& nick) const;
    void    registerNick(const std::string& nick, int fd);
    void    unregisterNick(const std::string& nick);

    // Channel access. All channel-aware logic lives in the command handlers;
    // these are convenience helpers that do the common case.
    bool    sendToChannel(const std::string& channelName,
                          const std::string& line,
                          int exceptFd = -1);

    // Channel management helpers used by command handlers
    Channel* getOrCreateChannel(const std::string& name);
    Channel* getChannel(const std::string& name);
    bool     channelExists(const std::string& name) const;

    bool     isMember(const std::string& channel, const std::string& nick) const;
    bool     isOperator(const std::string& channel, const std::string& nick) const;

    bool     addMemberToChannel(const std::string& channel, int fd, const std::string& key = "");
    bool     removeMemberFromChannel(const std::string& channel, const std::string& nick);

    void     setChannelInviteOnly(const std::string& channel, bool value);
    void     setChannelTopicRestricted(const std::string& channel, bool value);
    void     setChannelOperator(const std::string& channel, const std::string& nick, bool value);
    void     setChannelKey(const std::string& channel, const std::string& key);
    void     clearChannelKey(const std::string& channel);
    void     setChannelUserLimit(const std::string& channel, size_t limit);
    void     clearChannelUserLimit(const std::string& channel);

    void     addInvite(const std::string& channel, const std::string& nick);
    void     replyChannelMode(int fd, const std::string& channel);

    void    sendToClient(int fd, const std::string& line);
    void    flushToClient(int fd);
    void    disconnectClient(int fd);
    void    tryRegister(int fd);
    void    broadcastToMemberChannels(int fd, const std::string& line);
    void    removeClientFromChannels(int fd);

private:
    int         _listenFd;
    int         _port;
    std::string _password;

    std::vector<struct pollfd>      _pollFds;
    std::map<int, Client>           _clients;

    std::unordered_map<std::string, int> _nickToFd;
    std::map<std::string, Channel>       _channels;  // case-insensitive comparator (TBD by partner)

    std::unordered_map<std::string_view, Command> dispatchCommand;

    void    buildDispatch();
    void    acceptNewClient();
    void    readFromClient(int fd);

    void    handleCommand(int fd, const std::string& line);
    Message parseMessage(const std::string& line);

    void    closeFds();
    void    clearClients(int fd);

    static volatile sig_atomic_t _signalReceived;
    static void signalHandler(int signum);
};
