CC:=gcc
FLAGS:=-Werror -Wall -pedantic -Wextra
SOURCES:=helper.c editor.c piece_table.c renderer.c terminal_mod.c main.c
EX:=editor

build:
	$(CC) $(FLAGS) $(SOURCES) -o $(EX)
run:
	./$(EX)
clean:
	rm -f $(EX)
