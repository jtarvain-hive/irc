#pragma once

#include <string>

class Client {
public:
	Client();
	Client(int fd, const std::string& ipAddr);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();

	int					getFd() const;
	const std::string&	getIpAddr() const;

	void				setFd(int fd);
	void				setIpAddr(const std::string& ipAddr);

private:
	int			_fd;		// client socket file descriptor
	std::string	_ipAddr;	// client IP address (dotted-decimal)
};
