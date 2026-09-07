#pragma once

#include <string>

class Client {
public:
	Client(int fd, const std::string& ipAddr);
	Client(const Client& other) = delete;
	Client& operator=(const Client& other) = delete;
	~Client();

	int					getFd() const;
	const std::string&	getIpAddr() const;
	const std::string&	getNick() const;
	const std::string&	getPass() const;
	const std::string&	getUser() const;

	bool				isRegistered() const;
	bool				gotPass() const;
	bool				gotNick() const;
	bool				gotUser() const;


	void				setFd(int fd);
	void				setIpAddr(const std::string& ipAddr);
	void				setNick(const std::string& nick);
	void				setPass(const std::string& pass);
	void				setUser(const std::string& user, const std::string& realname);
	void				setRegistered(bool value);

	std::string&		recvBuf();
	std::string&		outBuf();

private:
	int			_fd;		// client socket file descriptor
	std::string	_ipAddr;	// client IP address (dotted-decimal)

	bool		_gotPass;
	bool		_gotNick;
	bool		_gotUser;

	bool		_registered;

	std::string	_nick;
	std::string	_pass;
	std::string	_username;



	std::string _recvBuf; // recv()
	std::string	_outBuf; // send()
};