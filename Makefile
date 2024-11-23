CC = gcc
CFLAGS = -Wall
LFLAGS = -lraylib

API_TOK ?= YOUR_TOKEN

NAME = jpx

all: src/*.c
	$(CC) $(CFLAGS) -o $(NAME) $^ $(LFLAGS)
