#ifndef TILE_REQ_H
#define TILE_REQ_H

#include <raylib.h>
#include <pthread.h>
#include "map.h"

#define MAX_DL_THREADS 12
#define MAX_TILES_PER_REQUEST 24
#define BATCH_SIZE 12

typedef enum {
    TILE_READY,
    TILE_LOADED,
    TILE_NOT_READY,
} TileDataStatus;

typedef struct {
    Image tile_img;
    Texture texture;
    TileDataStatus status; 
    long last_accessed; // TODO: implement cache eviction
} TileData;

typedef struct {
    Tile key; 
    TileData value;
} Item;

typedef struct {
    Tile *tiles;
    int tile_count;
} TileRequest;

typedef struct {
    TileRequest request;
    void (*call_back)(void);
    Item **tile_cache; // cache hash map
    pthread_mutex_t *mutex;
} DownloadContext;

void start_download_thread(TileRequest request, Item **tile_cache, pthread_mutex_t *mutex, void (*call_back)(void));

#endif
