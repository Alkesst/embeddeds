#/bin/bash
gcc -Wall -Wextra -Werror -o "$1".out "$1".c -lpthread -lrt -fsanitize=address -fsanitize=undefined -g
exit $?
