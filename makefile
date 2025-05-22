options = -Wall -Wextra -Wpedantic -Werror -g
build:
	gcc $(options) -lraylib main.c -o main
