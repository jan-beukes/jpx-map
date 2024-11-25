#ifndef TILE_REQ_H
#define TILE_REQ_H

#include <raylib.h>
#include "map.h"

typedef enum {
    TILE_READY,
    TILE_LOADED,
    NOT_READY,
} TileDataStatus;

typedef struct {
    Image tile_img;
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
    void *user_data;
    Item **tile_cache; // cache hash map
} DownloadContext;

void start_download_thread(TileRequest request, Item **tile_cache, void *userdata);

#endif
