#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
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

#define SCREEN_TILE_COUNT 8 // desired tile count
#define MAX_ZOOM 19
#define MIN_ZOOM 4
#define ZOOM_SPEED 10

#define SCREEN_TL(__s) ((Coord){__s.min.x, __s.max.y})
#define SCREEN_BR(__s) ((Coord){__s.max.x, __s.min.y})

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

MapBB get_screen_bb(Coord top_left, double width) {
    MapBB b;
    b.min.x = top_left.x;
    b.max.y = top_left.y;
    b.max.x = top_left.x + width;
    b.min.y = top_left.y - width*A_RATIO;
    return b;
}

Vector2 point_to_screen(Coord p, MapBB screen) {
    double width = screen.max.x - screen.min.x;
    double height = screen.max.y - screen.min.y;

    double x_ratio = (p.x - screen.min.x) / width;
    double y_ratio = (screen.max.y - p.y) / height;

    return (Vector2) {
        .x = x_ratio * SCREEN_WIDTH,
        .y = y_ratio * SCREEN_HEIGHT,
    };
}

Coord screen_to_coord(Vector2 screen_pos, MapBB screen) {
    double width = screen.max.x - screen.min.x;
    double height = screen.max.y - screen.min.y;

    double x_coord = screen.min.x + screen_pos.x * (width / SCREEN_WIDTH); 
    double y_coord = screen.max.y - screen_pos.y * (height / SCREEN_HEIGHT);

    return (Coord) {x_coord, y_coord};
}

Rectangle tile_screen_rect(MapBB screen, Tile t) {
    // mercator pixel_coords
    Vector2 pixel_topleft = coord_to_pixel_space(SCREEN_TL(screen), t.zoom);
    Vector2 pixel_bottomright = coord_to_pixel_space(SCREEN_BR(screen), t.zoom);

    double scale = fmax(
            SCREEN_WIDTH / (pixel_bottomright.x - pixel_topleft.x),
            SCREEN_HEIGHT / (pixel_bottomright.y - pixel_topleft.y)
    );

    double tile_pixel_x = t.x * 256;
    double tile_pixel_y = t.y * 256;

    float tile_x_screen = (tile_pixel_x - pixel_topleft.x) * scale;
    float tile_y_screen = (tile_pixel_y - pixel_topleft.y) * scale;
    float tile_width = 256 * scale;
    float tile_height = 256 * scale;

    return (Rectangle){tile_x_screen, tile_y_screen, tile_width, tile_height};
}

#define FALLBACK_LIMIT 6
void pre_cache_tiles(MapBB screen, int zoom, Item **tile_cache_ptr) {
    int min_zoom = zoom - FALLBACK_LIMIT;
    min_zoom = min_zoom < MIN_ZOOM ? MIN_ZOOM : min_zoom;

    Tile request_tiles[MAX_TILES_PER_REQUEST];
    int tile_req_count = 0;
    for (int z = zoom - 2; z > min_zoom; z -= 2) {
        Tile min = coord_to_tile(SCREEN_TL(screen), z);
        Tile max = coord_to_tile(SCREEN_BR(screen), z);

        for (int y = min.y; y <= max.y; y++) {
            for (int x = min.x; x <= max.x; x++) {
                Tile t = {z, x, y};
                if (tile_req_count < MAX_TILES_PER_REQUEST) {
                    request_tiles[tile_req_count++] = t;
                }
            }
        }

    }
    if (tile_req_count > 0) {
        increment_threads();
        TileRequest request = {
            .tiles = request_tiles,
            .tile_count = tile_req_count,
        };
        start_download_thread(request, tile_cache_ptr, &mutex, decrement_threads);
    }
}

void render_fallback_tile(Tile t, MapBB screen, Item **tile_cache) {
    // lower res tiles
    for (int i = 0; i < FALLBACK_LIMIT; i++) {
        t.zoom -= 1;
        t.x /= 2;
        t.y /= 2;
        Item *item;
        if ((item = hmgetp_null(*tile_cache, t)) == NULL) continue;
        if (item->value.status == TILE_NOT_READY) continue;

        if (item->value.status == TILE_LOADED) {
            item->value.status = TILE_READY;
            item->value.texture = LoadTextureFromImage(item->value.tile_img);
        }
        // found fallback
        Texture texture = item->value.texture;
        Rectangle rec = tile_screen_rect(screen, t);
        Rectangle src = {0, 0, texture.width, texture.height};
        DrawTexturePro(texture, src, rec, (Vector2){0,0}, 0, WHITE);
        //DrawRectangleLinesEx(rec, 1, DARKGREEN);
        return;
    }

}

void render_tiles(MapBB screen, int zoom, Item **tile_cache_ptr) {
    Item *tile_cache = *tile_cache_ptr;

    Tile min = coord_to_tile(SCREEN_TL(screen), zoom);
    Tile max = coord_to_tile(SCREEN_BR(screen), zoom);

    Tile request_tiles[MAX_TILES_PER_REQUEST];
    int tile_req_count = 0;

    int cap = (max.x - min.x + 1) * (max.y - min.y + 1);
    Item ready_tiles[cap];
    int ready_tile_count = 0;

    int unloaded = 0;
    for (int y = min.y; y <= max.y; y++) {
        for (int x = min.x; x <= max.x; x++) {
            Tile t = {zoom, x, y};
            bool fallback = false;

            Item *item = hmgetp_null(tile_cache, t);
            if (item == NULL) {
                // not cached
                if (tile_req_count < MAX_TILES_PER_REQUEST) request_tiles[tile_req_count++] = t;
                fallback = true;
            } else if (item->value.status == TILE_NOT_READY){
                unloaded++;
                fallback = true;
            } else if (item->value.status == TILE_LOADED) {
                item->value.status = TILE_READY;
                item->value.texture = LoadTextureFromImage(item->value.tile_img);
            }

            // desired tile is not available
            if (fallback) {
                render_fallback_tile(t, screen, tile_cache_ptr);
                continue;
            }
            assert(item->value.status == TILE_READY);
            ready_tiles[ready_tile_count++] = *item;
        }
    }
    // Render ready tiles after all fallback tiles
    for (int i = 0; i < ready_tile_count; i++) {
        Item item = ready_tiles[i];
        Texture texture = item.value.texture;
        Rectangle rec = tile_screen_rect(screen, item.key);
        Rectangle src = {0, 0, texture.width, texture.height};
        DrawTexturePro(texture, src, rec, (Vector2){0,0}, 0, WHITE);
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

MapBB move_screen(MapBB screen, float dt, int *zoom) {
    int z = *zoom;

    //---Zooming---
    float wheel_dir = 0;
    if ((wheel_dir = GetMouseWheelMove()) > 0 && z < MAX_ZOOM
        || wheel_dir < 0 && z > MIN_ZOOM) { 

        double width = screen.max.x - screen.min.x;
        if (wheel_dir > 0) {
            float zoom = (1 - dt * ZOOM_SPEED);
            zoom = zoom < 0 ? 0 : zoom;
            width *= zoom;
        } else {
            float zoom = (1 + dt * ZOOM_SPEED);
            width *= zoom;
        }
        double height = width * A_RATIO;

        Vector2 mouse_pos = GetMousePosition();

        Coord prev_mouse_coord = screen_to_coord(mouse_pos, screen);
        screen.max.x = screen.min.x + width;
        screen.max.y = screen.min.y + height;
        Coord current_mouse_coord = screen_to_coord(mouse_pos, screen);
        Coord displacement = {
            .x = prev_mouse_coord.x - current_mouse_coord.x,
            .y = prev_mouse_coord.y - current_mouse_coord.y,
        };
        screen.min.x += displacement.x;
        screen.max.x += displacement.x;
        screen.min.y += displacement.y;
        screen.max.y += displacement.y;

        z = ZOOM_FROM_WIDTH(width/SCREEN_TILE_COUNT);
    }

    // Movement
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        Vector2 delta = GetMouseDelta();

        double x_scale = (screen.max.x - screen.min.x) / SCREEN_WIDTH;
        double y_scale = (screen.max.y - screen.min.y) / SCREEN_HEIGHT;

        Coord coord_delta = {
            .x = delta.x * x_scale,
            .y = delta.y * y_scale,
        };
        // clamp screen
        if (screen.min.x - coord_delta.x > -180 && screen.max.x - coord_delta.x < 180) {
            screen.min.x -= coord_delta.x;
            screen.max.x -= coord_delta.x;
        }
        if (screen.min.y + coord_delta.y > -85.0511 && screen.max.y + coord_delta.y < 85.0511) {
            screen.min.y += coord_delta.y;
            screen.max.y += coord_delta.y;
        }

    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    *zoom = z;
    return screen;
}

int main(int argc, char *argv[]) {

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "jpx");
    SetTargetFPS(144);

    // Cache hash map
    Item *tile_cache = NULL;
    pthread_mutex_init(&mutex, NULL);

    Coord tl = {18.84587, -33.9225};
    double width = 0.0699;
    MapBB screen = get_screen_bb(tl, width);

    int zoom = ZOOM_FROM_WIDTH(width/SCREEN_TILE_COUNT);

    // TODO: Pre cache large BB around starting BB
    pre_cache_tiles(screen, zoom, &tile_cache);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        screen = move_screen(screen, dt, &zoom);

        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        render_tiles(screen, zoom, &tile_cache);

        DrawText(TextFormat("Active Threads %d", active_download_threads), 10, 10, 30, BLUE);
        DrawText(TextFormat("Zoom: %d", zoom), 10, 90, 30, BLUE);
        DrawText(TextFormat("Top Left: %.2lf %.2lf", screen.min.x, screen.max.y), 10, 130, 20, RED);
        DrawText(TextFormat("Bottom Right: %.2lf %.2lf", screen.max.x, screen.min.y), 10, 160, 20, RED);

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
