#ifndef MAP_H
#define MAP_H

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

typedef struct {
    const char *filename;
    Coord *points; //(long, lat)
    int *hr_values;
    size_t num_points;
} GpxData;

Tile coord_to_tile(Coord p, int z);
Coord tile_to_coord(Tile t);
Vector2 coord_to_pixel_space(Coord p, int zoom);

GpxData get_gpx_data(const char *file_name);
void free_gpx_data(GpxData data);

#endif
