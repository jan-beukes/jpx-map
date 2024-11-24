CC = gcc
CFLAGS = -Wall
LFLAGS = -L libs -lcurl -lraylib -lm

API_TOK ?= pk.eyJ1IjoiZGlzdHVyYmVkLXdhZmZsZSIsImEiOiJjbTN1cTFuMHAwZ3RwMmtzZmV1ajRtcmF3In0.VrF87lJUjSEaoZCSBK37RQ
NAME = jpx

all: src/*.c
	$(CC) $(CFLAGS) -o $(NAME) $^ -DAPI_TOK=\"$(API_TOK)\" $(LFLAGS)
