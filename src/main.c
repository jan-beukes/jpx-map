#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include "map.h"
#include "tile_req.h"
#include "stb_ds.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define A_RATIO ((double)SCREEN_HEIGHT/SCREEN_WIDTH)

#define CACHE ".cache"

#define MAX_CACHED_TILES 1000
#define MAX_DL_THREADS 8
#define MAX_TILES_PER_REQUEST 32

#define SCREEN_TILE_COUNT 6 // desired tile count

// glob
atomic_int active_download_threads = 0;
pthread_mutex_t mutex;

void increment_threads() {
    atomic_fetch_add(&active_download_threads, 1);
}
void decrement_threads() {
    atomic_fetch_sub(&active_download_threads, 1);
    assert(atomic_load(&active_download_threads) >= 0);
}

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

#define FALLBACK_LIMIT 4
Texture *get_fallback_tile_texture(Tile *t, Item **tile_cache) {
    for (int i = 0; i < FALLBACK_LIMIT; i++) {
        t->zoom -= 1;
        t->x /= 2;
        t->y /= 2;
        Item *item;
        if ((item = hmgetp_null(*tile_cache, *t)) == NULL) continue;
        if (item->value.status != TILE_READY) continue;

        return &item->value.texture;
    }

    return NULL;
}

void render_tiles(MapBB screen, int zoom, Item **tile_cache_ptr) {
    Item *tile_cache = *tile_cache_ptr;

    Tile min = point_to_tile(screen.min, zoom);
    Tile max = point_to_tile(screen.max, zoom);

    Tile request_tiles[MAX_TILES_PER_REQUEST];
    int tile_req_count = 0;
    
    int unloaded = 0;
    for (int y = min.y; y <= max.y; y++) {
        for (int x = min.x; x <= max.x; x++) {
            Tile t = {zoom, x, y};
            bool fallback = false;

            // not cached
            if (hmgeti(tile_cache, t) == -1) {
                if (tile_req_count < MAX_TILES_PER_REQUEST) request_tiles[tile_req_count++] = t;
                fallback = true;
            } 
            TileData *data = &hmgetp(tile_cache, t)->value;
            if (data->status == TILE_NOT_READY) {
                unloaded++;
                fallback = true;
            }
            else if (data->status == TILE_LOADED) {
                data->status = TILE_READY;
                data->texture = LoadTextureFromImage(data->tile_img);
            }

            Texture texture;
            if (fallback) {
                Texture *ptr = get_fallback_tile_texture(&t, tile_cache_ptr);
                if (ptr == NULL) continue;
                texture = *ptr;
            } else {
                texture = data->texture;
            }

            Rectangle rec = tile_screen_rect(screen, t);
            Rectangle src = {0, 0, texture.width, texture.height};
            DrawTexturePro(texture, src, rec, (Vector2){0,0}, 0, WHITE);
        }
    }

    DrawText(TextFormat("Not Loaded %d", unloaded), 10, 50, 30, BLUE);

    if (atomic_load(&active_download_threads) < MAX_DL_THREADS && tile_req_count > 0) {
        increment_threads();
        TileRequest request = {
            .tiles = request_tiles,
            .tile_count = tile_req_count,
        };
        start_download_thread(request, tile_cache_ptr, &mutex, decrement_threads);
    }

}

int main(int argc, char *argv[]) {

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "jpx");

    // Cache hash map
    Item *tile_cache = NULL;
    pthread_mutex_init(&mutex, NULL);

    Point tl = {18.84587, -33.9225};
    double width = 0.0699;
    MapBB screen = get_screen_bb(tl, width);

    int min_zoom = ZOOM_FROM_WIDTH(width) - 1;
    int max_zoom = ZOOM_FROM_WIDTH(width/SCREEN_TILE_COUNT);
    int zoom = min_zoom;

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_EQUAL) && zoom < 19) {
            zoom++;
        } else if (IsKeyPressed(KEY_MINUS) && zoom > 0) {
            zoom--;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        render_tiles(screen, zoom, &tile_cache);
        DrawText(TextFormat("Active Threads %d", active_download_threads), 10, 10, 30, BLUE);

        EndDrawing();
    }
    printf("HM length: %ld\n", hmlen(tile_cache));

    pthread_mutex_destroy(&mutex);

    // Unload cached tiles
    for (int i = 0; i < hmlen(tile_cache); i++) {
        TileData data = tile_cache[i].value;
        if (data.status == TILE_LOADED) UnloadImage(data.tile_img);
        if (data.status == TILE_READY) UnloadTexture(data.texture);
    }
    CloseWindow();

    return 0;
}
