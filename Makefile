CXX = gcc
CXXFLAGS = -Wall -Wextra -g# Mettre -O1 ou -O2 à la place de -g pour la version prod
HEADERS_LOCALISATION = include

LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image # -mwindows pour supprimer l'affichage de la console
LIB_LOCALISATION = lib

EXEC = Z_CHIP-8_Emulator

SRC = main.c chip8.c utils.c
OBJ = $(SRC:.c=.o)

all : program

program : $(OBJ)
	$(CXX) -L $(LIB_LOCALISATION) $(addprefix obj\, $(OBJ)) -o bin\$(EXEC) $(LDFLAGS)

%.o: src\%.c
	$(CXX) $(CXXFLAGS) -I $(HEADERS_LOCALISATION) -c $< -o obj\$@

clean:
	del /f /q obj\*.o

mrproper: clean
	del /f /q bin\$(EXEC).exe
	
.PHONY: all program clean mrproper