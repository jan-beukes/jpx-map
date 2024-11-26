#include <math.h>
#include <raylib.h>
#include "map.h"

#define MAX(_X, _Y) (_X) > (_Y) ? (_X) : (_Y)
#define ZOOM_FROM_SIZE(_W) (int)log2(360/(_W))

Tile coord_to_tile(Coord p, int z) { 
    Tile t;
    t.zoom = z;
    t.x = (int)(floor((p.x + 180.0) / 360.0 * (1 << z))); 
    double latrad = p.y * M_PI/180.0;
    t.y = (int)(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z))); 
    return t;
}

Coord tile_to_coord(Tile t) {
    Coord p;
    p.x = t.x / (double)(1 << t.zoom) * 360.0 - 180;
    double n = M_PI - 2.0 * M_PI * t.y / (double)(1 << t.zoom);
    p.y = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    return p;
}

// Function to convert longitude/latitude to Mercator pixel coordinates
Vector2 coord_to_pixel_space(Coord p, int zoom) {
    Vector2 v;
    int scale = 256 * (1 << zoom); // Total size of the map at this zoom level
    v.x = scale * (p.x + 180.0) / 360.0;
    double lat_rad = fmax(-85.0511, fmin(85.0511, p.y)) * M_PI / 180.0; // Clamp latitude
    v.y = scale * (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0;

    return v;
}

// GpxData get_gpx_data(const char *file_name) {
// 
// }
// 
// void free_gpx_data(GpxData data) {
// 
// }
