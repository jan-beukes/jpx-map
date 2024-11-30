#ifndef TILE_REQ_H
#define TILE_REQ_H

#include <raylib.h>
#include <pthread.h>
#include "map_renderer.h"

#define CACHE ".cache"
#define TILE_SET "osm"
#define FORMAT ".png"
#define URL "https://tile.openstreetmap.org/%d/%d/%d%s"
//#define TILE_SET "mapbox.satellite"
//"https://api.mapbox.com/v4/%s/%d/%d/%d.jpg?access_token=%s"

#define MAX_DL_THREADS 8
#define MAX_TILES_PER_REQUEST 64
#define BATCH_SIZE 16

typedef struct {
    Tile *tiles;
    int tile_count;
} TileRequest;

typedef struct {
    TileRequest request;
    void (*call_back)(void);
    Item **tile_cache; // cache hash map
    pthread_mutex_t *mutex;
} ThreadContext;

// 
void fetch_tiles(TileRequest request, Item **tile_cache, pthread_mutex_t *mutex, void (*call_back)(void));
Item *load_tile_from_file(Tile t, Item *tile_cache);

#endif
