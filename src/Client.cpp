#include "Client.hpp"

Client::Client() : _fd(-1), _ipAddr("") {}

Client::Client(int fd, const std::string& ipAddr) : _fd(fd), _ipAddr(ipAddr) {}

Client::Client(const Client& other) : _fd(other._fd), _ipAddr(other._ipAddr) {}

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_fd = other._fd;
		_ipAddr = other._ipAddr;
	}
	return (*this);
}

Client::~Client() {}

int Client::getFd() const {
	return (_fd);
}

const std::string& Client::getIpAddr() const {
	return (_ipAddr);
}

void Client::setFd(int fd) {
	_fd = fd;
}

void Client::setIpAddr(const std::string& ipAddr) {
	_ipAddr = ipAddr;
}
