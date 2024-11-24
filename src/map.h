#include <raylib.h>
#include <stdlib.h>

typedef struct {
    int x;
    int y;
    int zoom;
} Tile;

typedef struct {
    double x;
    double y;
} Point;

typedef struct {
    const char *filename;
    Point *points; //(long, lat)
    int *hr_values;
    size_t num_points;
} GpxData;

typedef struct {
    Point min;
    Point max;
} MapBB;

typedef struct {
    const char save_dir_name;
    MapBB bb;
    int base_zoom;
} MapData;


void fetch_map_tiles(GpxData gpx_data);

GpxData get_gpx_data(const char *file_name);
void free_gpx_data(GpxData data);

