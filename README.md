# Text Editor

This is a terminal based text editor written purely in C using mostly system calls and is powered by a data structure called `piece table`. The `piece table` data structure makes it easy to edit text documents and it is the structure used by vs code.

## Features
- Open and edit text files
- Save
- Quit
- Cursor movement with line wrapping 
- Backspace with line joining
- Page scrolling
- Enter key, new line creation
- Character insertion
- Status bar

## Dependencies
You need to have a C compiler and make installed on your system:
```bash
sudo apt install gcc make
```

## How to Compile
```bash
make build
```

## How to run
```bash
./editor <filename>   # With file
# Without file
./editor
```

## Key Bindings
- Arrow keys - navigate cursor
- Backspace - delete character before cursor, joins lines at beginning of line
- Delete - delete character at cursor
- Enter - create a new line
- Page Up/ Page Down - Scroll through document
- Ctrl + q - Quit the editor
- Ctrl + s - Save the file
- Status bar
- Search with highlighting

## Project structure
```bash
editor.c						# Editor manipulation API
editor.h						# Editor header file
helper.c						# Helper functions
helper.h						# Helpers header file
main.c							# Entry point to the program
Makefile						# make instructions
piece_table.c				# Piece table structure API file
renderer.c					# Screen rendering API
renderer.h					# Screen rendering header file
terminal_mod.c			# Terminal modification API
terminal_mod.h			# Terminal modification header file
```

## How it Works
The architecture of this project is based on the data structure called `Piece Table`. The piece table is a structure with many pieces (a structure that contains the type of a buffer, the start index and the length), the original bytes buffer, an append buffer along with other metadata fields in the struct. So on start up, the original buffer holds what is already in the file while the append buffer holds any incoming data that is to be written to the file.