#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "stb_ds.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define CACHE ".cache"
#define TEST_FILE "goofy.gpx"

typedef struct {
    Vector2 *points;
    Texture *tiles;
    MapBB render_bb;
} MapRender;

typedef struct {
    Texture texture;
    unsigned long last_accessed;
} CacheEntry;

Texture get_tile_texture(Tile t, void *cached_tiles) {

}


int main(int argc, char *argv[]) {
    const char *file_name = TEST_FILE;

    if (argc > 1) {
        if (strcmp(argv[1], "--clear") == 0) {
            if (DirectoryExists(CACHE)) {
            system("rm -rf " CACHE "/*");
            }
            printf("Cleared Cache\n");
        }
        else {
            fprintf(stderr, "Invalid arguments\n");
            exit(1);
        }
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "jpx");

    GpxData gpx_data = get_gpx_data(file_name);
    struct {Tile key; CacheEntry value;} *cached_tiles = NULL;

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
