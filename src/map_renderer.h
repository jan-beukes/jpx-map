#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include <raylib.h>
#include <stdlib.h>

#define TILE_WIDTH(_zoom) (360.0/(1 << (_zoom)))
#define ZOOM_FROM_WIDTH(_w) ((int)(log2(360.0/(_w))))

typedef struct {
    int zoom;
    int x;
    int y;
} Tile;

typedef struct {
    double x;
    double y;
} Coord;

typedef struct {
    Coord min;
    Coord max;
} MapBB;

typedef enum {
    TILE_READY,
    TILE_LOADED,
    TILE_NOT_READY,
} TileDataStatus;

typedef struct {
    Image tile_img;
    Texture texture;
    TileDataStatus status; 
    long last_accessed; 
} TileData;

typedef struct {
    Tile key; 
    TileData value;
} Item;

typedef struct {
    int width;
    int height;
    MapBB screen;
    int zoom;
    Item *tile_cache;
    pthread_mutex_t mutex;
} Renderer;

Coord screen_to_coord(Renderer *r, Vector2 screen_pos);
Vector2 coord_to_screen(Renderer *r, Coord p);
MapBB get_rect_bb(Coord top_left, double width, double height);

void init_map_renderer(Renderer *r, MapBB screen, int width, int height);
void deinit_map_renderer(Renderer *r);

void render_tiles(Renderer *r);
void move_screen(Renderer *r);

#endif
