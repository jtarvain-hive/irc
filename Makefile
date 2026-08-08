MAKEFLAGS		+= -j

NAME			:= ircserv
HEADERS			:= $(wildcard include/*.hpp)

CXX				:= c++
CXXFLAGS		:= -Wall -Wextra -Werror -std=c++20
HFLAGS			:= -I./include

SRC				:= main.cpp Client.cpp Server.cpp Channel.cpp commands.cpp
OBJ				:= $(addprefix obj/, $(notdir $(SRC:%.cpp=%.o)))

VPATH			:= src

all: $(NAME)

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
