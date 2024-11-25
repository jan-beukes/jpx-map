#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "map.h"
#include "tile_req.h"
#include "stb_ds.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define A_RATIO ((double)SCREEN_HEIGHT/SCREEN_WIDTH)

#define CACHE ".cache"
#define MAX_CACHED_TILES 1000
#define MAX_DL_THREADS 8

#define SCREEN_TILE_COUNT 6 // desired tile count

// glob
int active_download_threads = 0;

MapBB get_screen_bb(Point top_left, double width) {
    MapBB b;
    b.min = top_left;
    b.max.x = top_left.x + width;
    b.max.y = top_left.y - width*A_RATIO;
    return b;
}

Rectangle tile_screen_rect(MapBB screen, Tile t) {
    Point tile_point = tile_to_point(t);
    double lon_offset = tile_point.x - screen.min.x;
    double lat_offset = tile_point.y - screen.min.y;

    float x_scale_factor = SCREEN_WIDTH/(screen.max.x - screen.min.x);
    float y_scale_factor = SCREEN_HEIGHT/(screen.max.y - screen.min.y);
    
#define MAGIC_FIX_NUMBER 1.2
    float x_off = lon_offset * x_scale_factor;
    float y_off = lat_offset * y_scale_factor * MAGIC_FIX_NUMBER;

    double size = TILE_WIDTH(t.zoom) * x_scale_factor;

    return (Rectangle){x_off, y_off, size, size};
}

void render_tiles(MapBB screen, int zoom, Item **tile_cache_ptr) {
    Item *tile_cache = *tile_cache_ptr;

    Tile min = point_to_tile(screen.min, zoom);
    Tile max = point_to_tile(screen.max, zoom);

    int capacity = (max.x - min.x + 1) * (max.y - min.y + 1);
    Tile request_tiles[capacity];
    int tile_req_count = 0;
    
    for (int y = min.y; y <= max.y; y++) {
        for (int x = min.x; x <= max.x; x++) {
            Tile t = {zoom, x, y};

            // not cached
            if (hmgeti(tile_cache, t) == -1) {
                request_tiles[tile_req_count++] = t;
                continue;
            } 

            TileData data = hmget(tile_cache, t);
            if (!data.ready) continue; // not done fetching texture

            Texture texture = LoadTextureFromImage(data.tile_img);
            Rectangle rec = tile_screen_rect(screen, t);
            Rectangle src = {0, 0, texture.width, texture.height};
            DrawTexturePro(texture, src, rec, (Vector2){0,0}, 0, WHITE);
        }
    }

    if (active_download_threads < MAX_DL_THREADS && tile_req_count > 0) {
        active_download_threads++;
        printf("Request %d started\n", active_download_threads);
        TileRequest request = {
            .tiles = request_tiles,
            .tile_count = tile_req_count,
        };
        start_download_thread(request, tile_cache_ptr, &active_download_threads);
    }


}

int main(int argc, char *argv[]) {

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "jpx");

    // Cache hash map
    Item *tile_cache = NULL;

    Point tl = {18.84587, -33.9225};
    double width = 0.0699;
    MapBB screen = get_screen_bb(tl, width);

    int min_zoom = ZOOM_FROM_WIDTH(width) - 1;
    int max_zoom = ZOOM_FROM_WIDTH(width/SCREEN_TILE_COUNT);
    int zoom = max_zoom;

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(BLACK);

        render_tiles(screen, zoom, &tile_cache);

        EndDrawing();

        // Increase res
        //if (zoom < max_zoom) {
        //    zoom++;
        //}
    }

    // Unload cached tiles
    for (int i = 0; i < hmlen(tile_cache); i++) {
        UnloadImage(tile_cache[i].value.tile_img);
    }
    CloseWindow();

    return 0;
}
