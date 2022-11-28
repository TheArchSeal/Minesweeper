.\build\mine.exe: minesweeper.c
	gcc .\minesweeper.c -o .\build\mine.exe -Wall -Wextra -Werror -pedantic -static -pipe -O2
