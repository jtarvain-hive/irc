MAKEFLAGS		+= -j

NAME			:= ircserv
HEADERS			:= include/Command.hpp include/Server.hpp include/Channel.hpp \
				   include/Client.hpp

CXX				:= c++
CXXFLAGS		:= -Wall -Wextra -Werror -std=c++20 -g
HFLAGS			:= -I./include

SRC				:= main.cpp Client.cpp Channel.cpp \
				   Server.cpp Dispatch.cpp Channels.cpp \
				   Cap.cpp Invite.cpp Join.cpp Kick.cpp Mode.cpp Nick.cpp Part.cpp \
				   Notice.cpp Pass.cpp Ping.cpp Privmsg.cpp Quit.cpp Topic.cpp User.cpp \
				   Who.cpp
OBJ				:= $(addprefix obj/, $(notdir $(SRC:%.cpp=%.o)))

VPATH			:= src src/Server src/Commands

all: $(NAME)

val: $(NAME)
	valgrind ./$(NAME) 6667 pw

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(HFLAGS) $(OBJ) -o $(NAME)

.NOTPARALLEL: obj
obj:
	mkdir -p obj

obj/%.o: %.cpp $(HEADERS) | obj
	$(CXX) $(CXXFLAGS) $(HFLAGS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

.NOTPARALLEL: re
re: fclean all

.PHONY: all clean fclean re
.SECONDARY: $(OBJ) obj
