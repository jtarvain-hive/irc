MAKEFLAGS		+= -j

NAME			:= ircserv
HEADERS			:= $(wildcard include/*.hpp) $(wildcard src/Commands/*.hpp)

CXX				:= c++
CXXFLAGS		:= -Wall -Wextra -Werror -std=c++20
HFLAGS			:= -I./include

SRC				:= $(shell find src -type f -name '*.cpp' | sort)
OBJ				:= $(patsubst src/%.cpp,obj/%.o,$(SRC))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(HFLAGS) $(OBJ) -o $(NAME)

obj/%.o: src/%.cpp $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(HFLAGS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

.NOTPARALLEL: re
re: fclean all

.PHONY: all clean fclean re
.SECONDARY: $(OBJ)
