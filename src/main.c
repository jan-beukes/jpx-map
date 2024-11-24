#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include "image_req.h"

#define MAP_WIDTH 1280
#define MAP_HEIGHT 720
#define MAP_STYLE "satellite-streets-v12"

#define CACHE ".cache"

#define TEST_FILE "test.gpx"
#define URL "https://api.mapbox.com/styles/v1/mapbox/%s/static/[%f,%f,%f,%f]/%dx%d?access_token="

int main() {

    const char *name = GetFileNameWithoutExt(TEST_FILE);
    if (!DirectoryExists(CACHE)) {
        MakeDirectory(CACHE);
    }

    // Handle cached map images
    Texture map_texture;
    if (!DirectoryExists(TextFormat("%s/%s", CACHE, name))) {
        MakeDirectory(TextFormat("%s/%s", CACHE, name));

        // Request
        Vector2 min = {18.8422, -33.9539};
        Vector2 max = {18.9232, -33.9161};

        const char *url = TextFormat(URL, MAP_STYLE, min.x, min.y, max.x, max.y, MAP_WIDTH, MAP_HEIGHT);


        MemChunk chunk = fetch_image(url, strlen(url));
        FILE *file = fopen(TextFormat("%s/%s/cached_map.png", CACHE, name), "w");
        if (!file) {
            perror("ERROR: Failed to open image from cache\n");
        } else {
            fwrite(chunk.memory, 1, chunk.size, file);
            fclose(file);
        }
        free(chunk.memory);
    } 
    
    InitWindow(1280, 720, "jpx");
    

    map_texture = LoadTexture(TextFormat("%s/%s/cached_map.png", CACHE, name));

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
