#include "Channel.hpp"

Channel::Channel(const std::string& name)
    : _name(name), _inviteOnly(false), _topicRestricted(false),
      _hasKey(false), _hasUserLimit(false), _userLimit(0)
{}

const std::string& Channel::getName() const
{
    return (_name);
}

const std::string& Channel::getTopic() const
{
    return (_topic);
}

void    Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

void    Channel::addMember(int fd)
{
    _members.insert(fd);
}

void    Channel::removeMember(int fd)
{
    _members.erase(fd);
    _operators.erase(fd);
}

bool    Channel::hasMember(int fd) const
{
    return (_members.count(fd) != 0);
}

bool    Channel::empty() const
{
    return (_members.empty());
}

const std::set<int>&   Channel::getMembers() const
{
    return (_members);
}

void    Channel::addOperator(int fd)
{
    _operators.insert(fd);
}

void    Channel::removeOperator(int fd)
{
    _operators.erase(fd);
}

bool    Channel::isOperator(int fd) const
{
    return (_operators.count(fd) != 0);
}

void    Channel::addInvite(int fd)
{
    _invited.insert(fd);
}

void    Channel::removeInvite(int fd)
{
    _invited.erase(fd);
}

bool    Channel::isInvited(int fd) const
{
    return (_invited.count(fd) != 0);
}

bool    Channel::isInviteOnly() const
{
    return (_inviteOnly);
}

void    Channel::setInviteOnly(bool value)
{
    _inviteOnly = value;
}

bool    Channel::isTopicRestricted() const
{
    return (_topicRestricted);
}

void    Channel::setTopicRestricted(bool value)
{
    _topicRestricted = value;
}

bool    Channel::hasKey() const
{
    return (_hasKey);
}

const std::string& Channel::getKey() const
{
    return (_key);
}

void    Channel::setKey(const std::string& key)
{
    _key = key;
    _hasKey = true;
}

void    Channel::clearKey()
{
    _key.clear();
    _hasKey = false;
}

bool    Channel::hasUserLimit() const
{
    return (_hasUserLimit);
}

size_t  Channel::getUserLimit() const
{
    return (_userLimit);
}

void    Channel::setUserLimit(size_t limit)
{
    _userLimit = limit;
    _hasUserLimit = true;
}

void    Channel::clearUserLimit()
{
    _hasUserLimit = false;
}
