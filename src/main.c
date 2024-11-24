#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define CACHE ".cache"
#define TEST_FILE "goofy.gpx"

typedef struct {
    Vector2 *points;
    Texture *tiles;
    MapBB render_bb;
} MapRender;

int main(int argc, char *argv[]) {
    /*if (argc == 1) {*/
    /*    fprintf(stderr, "Please provide gpx file");*/
    /*    exit(1);*/
    /*}*/
    /**/
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

    GpxData gpx_data = get_gpx_data(file_name);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "jpx");

    BeginDrawing();
    {
        const Color b = {0, 0, 0, 80};
        const int font_size = 40;
        const int width = MeasureText("Downloading...", font_size);
        Vector2 pos = {SCREEN_WIDTH/2.0f - width/2.0f, SCREEN_HEIGHT/2.0f - font_size/2.0f};
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, b);
        DrawText("Downloading...", pos.x, pos.y, font_size, RAYWHITE);
    }
    EndDrawing();


    MapBB bb = {(Vector2){17.3062, -34.5229}, (Vector2){19.8089, -33.355}};

    MapRender render = {
        .points = gpx_data.points,
    };

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(map_texture, 0, 0, WHITE);

        EndDrawing();
    }

    UnloadTexture(map_texture);
    CloseWindow();

    return 0;
}
