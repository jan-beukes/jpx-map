#include <raylib.h>
#include <stdlib.h>

#define MAP_BB_LIMIT_FACTOR 3
#define LEVELS_OF_ZOOM 3

typedef struct {
    const char *filename;
    Vector2 *points; //(long, lat)
    int *hr_values;
    size_t num_points;
} GpxData;

typedef struct {
    Vector2 min;
    Vector2 max;
} MapBB;

typedef struct {
    const char save_dir_name;
    MapBB bb;
    int base_zoom;
} MapData;

MapBB mapbb_from_borders(float lon_min, float lon_max, float lat_min, float lat_max);
int tile_zoom_from_bb(MapBB bb);

GpxData get_gpx_data(const char *file_name);
void free_gpx_data(GpxData data);

