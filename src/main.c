#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "map_renderer.h"
#include "tile_req.h"
#include "stb_ds.h"
#include "gpx_overlay.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
#define A_RATIO ((double)SCREEN_HEIGHT/SCREEN_WIDTH)

#define CACHE_LIFETIME_S 12
#define CLEANUP_TIMER 2

// remove cache entries past lifetime
void clean_cache(Item *tile_cache, int hm_len, double time) {

    for (int i = 0; i < hm_len; i++) {
        if (time - tile_cache[i].value.last_accessed > CACHE_LIFETIME_S) {
            switch (tile_cache[i].value.status) {
                case TILE_NOT_READY: continue;
                case TILE_LOADED:
                    UnloadImage(tile_cache[i].value.tile_img);
                    break;
                case TILE_READY: 
                    UnloadTexture(tile_cache[i].value.texture);
                    break;
            }
            int r = hmdel(tile_cache, tile_cache[i].key);
        }
    }

}

int main(int argc, char *argv[]) {
    SetTraceLogLevel(LOG_WARNING);

    // Load GPX file if provided as argument
    const char *gpx_filename = NULL;
    if (argc > 1) {
        gpx_filename = argv[1];
    } else {
        gpx_filename = "Afternoon_Run.gpx"; // Default GPX file
    }

    Coord tl = {18.84587, -33.9225};
    double width = 0.0699;
    MapBB screen = get_rect_bb(tl, width, width*A_RATIO);

    Renderer r;
    init_map_renderer(&r, screen, SCREEN_WIDTH, SCREEN_HEIGHT);

    Font font = LoadFontEx("notosans.ttf", 32, NULL, 0);
    const float font_size = font.baseSize;

    // Initialize and load GPX track
    GpxTrack track;
    init_gpx_track(&track, (Color){255, 0, 0, 200}, 3.0f); // Red semi-transparent line with 3px thickness
    
    bool gpx_loaded = false;
    if (gpx_filename) {
        gpx_loaded = load_gpx_file(&track, gpx_filename);
        if (gpx_loaded && track.count > 0) {
            // Center map on the first point of the GPX track
            Coord first_point = track.points[0];
            width = 0.05; // Adjust zoom level
            screen = get_rect_bb((Coord){first_point.x, first_point.y}, width, width*A_RATIO);
            r.screen = screen;
            r.zoom = ZOOM_FROM_WIDTH(width);
        }
    }

    // Tile Cache 
    double last_cleanup = GetTime();
    MakeDirectory(TextFormat("%s/%s", CACHE, TILE_SET));

    // Track display toggle
    bool show_track = true;

    while (!WindowShouldClose()) {
        // Toggle track display with 'T' key
        if (IsKeyPressed(KEY_T)) {
            show_track = !show_track;
        }

        // evict cache
        double time = GetTime();
        int hm_len = hmlen(r.tile_cache);
        if (time - last_cleanup > CLEANUP_TIMER) {
            clean_cache(r.tile_cache, hm_len, time);
            last_cleanup = GetTime();
        }

        move_screen(&r);

        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        render_tiles(&r);
        
        // Render GPX track if loaded and display is enabled
        if (gpx_loaded && show_track) {
            render_gpx_track(&r, &track);
        }

        // DEBUG
        DrawTextEx(font, TextFormat("Active Threads %d", get_thread_count()), (Vector2){10, 10}, font_size, 0, BLUE);

        Vector2 t_pos = {10, 50};
        DrawTextEx(font, TextFormat("Cached Tiles: %d", hm_len), t_pos, font_size, 0, BLUE);
        t_pos.y += 40;
        DrawTextEx(font, TextFormat("Zoom: %d", r.zoom), t_pos, font_size, 0, BLUE);
        t_pos.y += 40;

        Vector2 mouse_pos = GetMousePosition();
        Coord mouse_coord = screen_to_coord(&r, mouse_pos);
        DrawTextEx(font, TextFormat("Mouse coord: (%.2lf, %.2lf)", mouse_coord.x, mouse_coord.y), t_pos, font_size, 0, BLUE);
        t_pos.y += 50;
        DrawTextEx(font, "Screen:", t_pos, font_size, 0, RED);
        t_pos.y += 40;
        DrawTextEx(font, TextFormat("(%.2lf, %.2lf) (%.2lf, %.2lf)", r.screen.min.x, r.screen.min.y,
                                    r.screen.max.x, r.screen.max.y), t_pos, font_size, 0, RED);

        // Display GPX info
        if (gpx_loaded) {
            t_pos.y += 50;
            DrawTextEx(font, TextFormat("GPX Track: %s", show_track ? "ON [T to hide]" : "OFF [T to show]"), 
                       t_pos, font_size, 0, RED);
            t_pos.y += 40;
            DrawTextEx(font, TextFormat("Points: %d", track.count), t_pos, font_size, 0, RED);
        }

        EndDrawing();
    }

    // Cleanup
    if (gpx_loaded) {
        deinit_gpx_track(&track);
    }
    deinit_map_renderer(&r);

    return 0;
}
