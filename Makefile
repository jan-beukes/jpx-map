CC = gcc
CFLAGS = -Wall -g
LFLAGS = -L libs -lcurl -lraylib -lm -lpthread

API_TOK ?= YOUR_TOKEN
NAME = jpx
SRC = src/main.c src/map_renderer.c src/tile_req.c src/stb_ds.c src/gpx_overlay.c

all: $(SRC)
	$(CC) $(CFLAGS) -o $(NAME) $^ -DAPI_TOK=\"$(API_TOK)\" $(LFLAGS)
