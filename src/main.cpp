#include "Server.hpp"

#include <exception>
#include <iostream>
#include <algorithm>
#include <cctype>

#define MAXLEN 32

int parsePort(const std::string& str)
{
	size_t pos;
	int port = std::stoi(str, &pos);
	if (pos != str.size())
		throw std::invalid_argument("port has non numeric characters");
	if (port < 1 || port > 65535)
		throw std::out_of_range("port must be between 1 and 65535");

	return (port);
}

std::string	validatePassword(const std::string& pass)
{
	if (pass.empty())
		throw std::invalid_argument("password is empty");
	
	if (pass.length() > MAXLEN)
		throw std::invalid_argument("password is too long");
	
	if (!std::all_of(pass.begin(), pass.end(), [](unsigned char c){ return (std::isprint(c) && !std::isspace(c)); }))
		throw std::invalid_argument("password must contain only valid characters");

	return pass;
}

int	main(int argc, char *argv[]) {
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password> \n";
		return (EXIT_FAILURE);
	}

	try 
	{
		int port = parsePort(argv[1]);
		std::string pass = validatePassword(argv[2]);

		Server server(port, pass);

		server.init();
		server.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		return (1);
	}
	
	return (0);
}
