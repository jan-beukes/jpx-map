CC = gcc
CFLAGS = -Wall
LFLAGS = -L libs -lcurl -lraylib -lm

API_TOK ?= YOUR_TOKEN
NAME = jpx

all: src/*.c
	$(CC) $(CFLAGS) -o $(NAME) $^ -DAPI_TOK=\"$(API_TOK)\" $(LFLAGS)
