#/bin/bash
gcc -Wall -Wextra -Werror -o "$1" "$1".c -lpthread -lrt -fsanitize=address -fsanitize=undefined
exit $?
