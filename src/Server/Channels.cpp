#include "Server.hpp"
#include "Client.hpp"

#include <sstream>

Client* Server::getClient(int fd)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return nullptr;
    return &it->second;
}

int Server::findFdByNick(const std::string& nick) const
{
    std::unordered_map<std::string, int>::const_iterator it = _nickToFd.find(nick);
    return (it == _nickToFd.end()) ? -1 : it->second;
}

bool Server::isNickInUse(const std::string& nick) const
{
    return _nickToFd.find(nick) != _nickToFd.end();
}

void Server::registerNick(const std::string& nick, int fd)
{
    _nickToFd[nick] = fd;
}

void Server::unregisterNick(const std::string& nick)
{
    _nickToFd.erase(nick);
}

bool Server::sendToChannel(const std::string& channelName,
                           const std::string& line,
                           int exceptFd)
{
    std::map<std::string, Channel>::iterator it = _channels.find(channelName);
    if (it == _channels.end()) return false;
    Channel& ch = it->second;
    const std::set<int>& members = ch.getMembers();
    for (std::set<int>::const_iterator itr = members.begin(); itr != members.end(); ++itr) {
        int memberFd = *itr;
        if (memberFd == exceptFd) continue;
        sendToClient(memberFd, line);
    }
    return true;
}

Channel* Server::getOrCreateChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return &it->second;
    std::pair<std::map<std::string, Channel>::iterator, bool> res = _channels.emplace(name, Channel(name));
    return &res.first->second;
}

Channel* Server::getChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);
    if (it == _channels.end()) return nullptr;
    return &it->second;
}

bool Server::channelExists(const std::string& name) const
{
    return _channels.find(name) != _channels.end();
}

bool Server::isMember(const std::string& channel, const std::string& nick) const
{
    int fd = findFdByNick(nick);
    if (fd < 0) return false;
    std::map<std::string, Channel>::const_iterator it = _channels.find(channel);
    if (it == _channels.end()) return false;
    return it->second.hasMember(fd);
}

bool Server::isOperator(const std::string& channel, const std::string& nick) const
{
    int fd = findFdByNick(nick);
    if (fd < 0) return false;
    std::map<std::string, Channel>::const_iterator it = _channels.find(channel);
    if (it == _channels.end()) return false;
    return it->second.isOperator(fd);
}

bool Server::addMemberToChannel(const std::string& channel, int fd, const std::string& key)
{
    Channel* ch = getOrCreateChannel(channel);
    if (!ch) return false;
    // Key check
    if (ch->hasKey() && ch->getKey() != key)
        return false;
    // User limit
    if (ch->hasUserLimit() && ch->getMembers().size() >= ch->getUserLimit())
        return false;
    ch->addMember(fd);
    // If invited, consume invite
    if (ch->isInvited(fd)) ch->removeInvite(fd);
    return true;
}

bool Server::removeMemberFromChannel(const std::string& channel, const std::string& nick)
{
    int fd = findFdByNick(nick);
    if (fd < 0) return false;
    std::map<std::string, Channel>::iterator it = _channels.find(channel);
    if (it == _channels.end()) return false;
    Channel& ch = it->second;
    if (!ch.hasMember(fd)) return false;
    ch.removeMember(fd);
    if (ch.empty()) _channels.erase(it);
    return true;
}

void Server::setChannelInviteOnly(const std::string& channel, bool value)
{
    Channel* ch = getOrCreateChannel(channel);
    if (ch) ch->setInviteOnly(value);
}

void Server::setChannelTopicRestricted(const std::string& channel, bool value)
{
    Channel* ch = getOrCreateChannel(channel);
    if (ch) ch->setTopicRestricted(value);
}

void Server::setChannelOperator(const std::string& channel, const std::string& nick, bool value)
{
    int fd = findFdByNick(nick);
    if (fd < 0) return;
    Channel* ch = getOrCreateChannel(channel);
    if (!ch) return;
    if (value) ch->addOperator(fd); else ch->removeOperator(fd);
}

void Server::setChannelKey(const std::string& channel, const std::string& key)
{
    Channel* ch = getOrCreateChannel(channel);
    if (!ch) return;
    ch->setKey(key);
}

void Server::clearChannelKey(const std::string& channel)
{
    Channel* ch = getChannel(channel);
    if (!ch) return;
    ch->clearKey();
}

void Server::setChannelUserLimit(const std::string& channel, size_t limit)
{
    Channel* ch = getOrCreateChannel(channel);
    if (!ch) return;
    ch->setUserLimit(limit);
}

void Server::clearChannelUserLimit(const std::string& channel)
{
    Channel* ch = getChannel(channel);
    if (!ch) return;
    ch->clearUserLimit();
}

void Server::addInvite(const std::string& channel, const std::string& nick)
{
    int fd = findFdByNick(nick);
    if (fd < 0) return;
    Channel* ch = getOrCreateChannel(channel);
    if (!ch) return;
    ch->addInvite(fd);
}

void Server::replyChannelMode(int fd, const std::string& channel)
{
    Channel* ch = getChannel(channel);
    std::string modes = "+";
    std::string args;
    if (!ch) {
        sendToClient(fd, ":ircserv 324 * " + channel + " :+\r\n");
        return;
    }
    if (ch->isInviteOnly()) modes += 'i';
    if (ch->isTopicRestricted()) modes += 't';
    if (ch->hasKey()) { modes += 'k'; args += " " + ch->getKey(); }
    if (ch->hasUserLimit()) {
        modes += 'l';
        std::stringstream ss; ss << ch->getUserLimit();
        args += " " + ss.str();
    }
    sendToClient(fd, ":ircserv 324 * " + channel + " " + modes + args + "\r\n");
}

void Server::broadcastToMemberChannels(int fd, const std::string& line)
{
    std::set<int> delivered;
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        Channel& ch = it->second;
        if (ch.hasMember(fd)) {
            const std::set<int>& members = ch.getMembers();
            for (std::set<int>::const_iterator itr = members.begin(); itr != members.end(); ++itr) {
                int memberFd = *itr;
                if (memberFd == fd || delivered.count(memberFd))
                    continue;
                delivered.insert(memberFd);
                sendToClient(memberFd, line);
            }
        }
    }
}

void Server::removeClientFromChannels(int fd)
{
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ) {
        Channel& ch = it->second;
        if (ch.hasMember(fd))
            ch.removeMember(fd);
        if (ch.empty())
            it = _channels.erase(it);
        else
            ++it;
    }
}