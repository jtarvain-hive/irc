#pragma once
#include <string>
#include <set>

// Per-channel state only. Knows nothing about Server, fds beyond identity, or sockets.
class Channel {
public:
    Channel(const std::string& name);

    const std::string& getName() const;

    const std::string& getTopic() const;
    void                setTopic(const std::string& topic);

    void        addMember(int fd);       // fd joins the channel
    void        removeMember(int fd);    // fd leaves (PART/KICK/disconnect)
    bool        hasMember(int fd) const;
    bool        empty() const;           // true once last member leaves -> Server erases the channel
    const std::set<int>& getMembers() const;

    void        addOperator(int fd);     // MODE +o
    void        removeOperator(int fd);  // MODE -o
    bool        isOperator(int fd) const;

    void        addInvite(int fd);       // INVITE while channel is +i
    void        removeInvite(int fd);    // consumed once the invited fd joins
    bool        isInvited(int fd) const;

    bool        isInviteOnly() const;        // mode i
    void        setInviteOnly(bool value);

    bool        isTopicRestricted() const;   // mode t: only operators may TOPIC
    void        setTopicRestricted(bool value);

    bool        hasKey() const;              // mode k
    const std::string& getKey() const;
    void        setKey(const std::string& key);
    void        clearKey();

    bool        hasUserLimit() const;        // mode l
    size_t      getUserLimit() const;
    void        setUserLimit(size_t limit);
    void        clearUserLimit();

private:
    std::string   _name;
    std::string   _topic;
    std::set<int> _members;    // all fds currently in the channel
    std::set<int> _operators;  // subset of _members with operator privilege
    std::set<int> _invited;    // fds allowed to JOIN despite +i, cleared on join

    bool          _inviteOnly;
    bool          _topicRestricted;
    bool          _hasKey;
    std::string   _key;
    bool          _hasUserLimit;
    size_t        _userLimit;
};
