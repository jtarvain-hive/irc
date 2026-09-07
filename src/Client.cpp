#include "Client.hpp"
#include <unistd.h>   // close()

Client::Client(int fd, const std::string& ipAddr) : _fd(fd), _ipAddr(ipAddr),
								 _gotPass(false), _gotNick(false), _gotUser(false), _registered(false) {}

Client::~Client()
{
	if (this->_fd != -1)
		close(_fd);
}

int Client::getFd() const
{
	return (_fd);
}

const std::string& Client::getIpAddr() const
{
	return (_ipAddr);
}

const std::string& Client::getNick() const
{
	return (_nick);
}

const std::string& Client::getPass() const
{
	return (_pass);
}

const std::string&	Client::getUser() const
{
	return (_username);
}


bool	Client::isRegistered() const
{
	return (_registered);
}

bool	Client::gotPass() const
{
	return (_gotPass);
}

bool	Client::gotNick() const
{
	return (_gotNick);
}

bool	Client::gotUser() const
{
	return (_gotUser);
}

void	Client::setFd(int fd)
{
	_fd = fd;
}

void	Client::setIpAddr(const std::string& ipAddr)
{
	_ipAddr = ipAddr;
}

void	Client::setNick(const std::string& nick)
{
	this->_nick = nick;
	this->_gotNick = true;
}

void	Client::setPass(const std::string& pass)
{
	this->_pass = pass;
	this->_gotPass = true;
}

void	Client::setUser(const std::string& user, const std::string& realname)
{
	(void)realname; // For now, we don't use the realname, but it's good to have.
	this->_username = user;
	this->_gotUser = true;
}

void	Client::setRegistered(bool value) {
	this->_registered = value;
}

std::string&	Client::recvBuf() {
	return (this->_recvBuf);
}

std::string&	Client::outBuf() {
	return (this->_outBuf);
}