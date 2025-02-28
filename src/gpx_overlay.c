#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>
#include "gpx_overlay.h"
#include "stb_ds.h"

#define GPX_INITIAL_CAPACITY 256
#define BUFFER_SIZE 1024

void init_gpx_track(GpxTrack *track, Color color, float thickness) {
    track->points = (Coord *)malloc(GPX_INITIAL_CAPACITY * sizeof(Coord));
    track->count = 0;
    track->capacity = GPX_INITIAL_CAPACITY;
    track->color = color;
    track->thickness = thickness;
}

void deinit_gpx_track(GpxTrack *track) {
    if (track->points) {
        free(track->points);
        track->points = NULL;
    }
    track->count = 0;
    track->capacity = 0;
}

static void add_point(GpxTrack *track, Coord point) {
    // Resize if needed
    if (track->count >= track->capacity) {
        track->capacity *= 2;
        track->points = (Coord *)realloc(track->points, track->capacity * sizeof(Coord));
    }
    
    track->points[track->count++] = point;
}

bool load_gpx_file(GpxTrack *track, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening GPX file: %s\n", filename);
        return false;
    }
    
    char buffer[BUFFER_SIZE];
    double lat, lon;
    
    while (fgets(buffer, BUFFER_SIZE, file)) {
        // Parse trkpt elements which contain lat/lon data
        if (strstr(buffer, "<trkpt")) {
            char *lat_str = strstr(buffer, "lat=\"");
            char *lon_str = strstr(buffer, "lon=\"");
            
            if (lat_str && lon_str) {
                lat_str += 5; // Skip past "lat=""
                lon_str += 5; // Skip past "lon=""
                
                lat = atof(lat_str);
                lon = atof(lon_str);
                
                Coord point = {lon, lat}; // Note: Coord is {x,y} which maps to {lon,lat}
                add_point(track, point);
            }
        }
    }
    
    fclose(file);
    printf("Loaded %d points from GPX file\n", track->count);
    return true;
}

void render_gpx_track(Renderer *r, GpxTrack *track) {
    if (track->count < 2) return; // Need at least 2 points to draw a line
    
    for (int i = 0; i < track->count - 1; i++) {
        Vector2 start = coord_to_screen(r, track->points[i]);
        Vector2 end = coord_to_screen(r, track->points[i+1]);
        
        // Skip line segments that are outside the screen
        if ((start.x < 0 && end.x < 0) || 
            (start.x > r->width && end.x > r->width) ||
            (start.y < 0 && end.y < 0) || 
            (start.y > r->height && end.y > r->height)) {
            continue;
        }
        
        DrawLineEx(start, end, track->thickness, track->color);
    }
}