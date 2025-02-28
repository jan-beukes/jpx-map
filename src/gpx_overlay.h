#ifndef GPX_OVERLAY_H
#define GPX_OVERLAY_H

#include <raylib.h>
#include "map_renderer.h"

typedef struct {
    Coord *points;    // Array of lat/lon coordinates
    int count;        // Number of points
    int capacity;     // Allocated capacity
    Color color;      // Line color
    float thickness;  // Line thickness
} GpxTrack;

// Initialize a new GPX track
void init_gpx_track(GpxTrack *track, Color color, float thickness);

// Free resources used by a GPX track
void deinit_gpx_track(GpxTrack *track);

// Parse a GPX file and load track points
bool load_gpx_file(GpxTrack *track, const char *filename);

// Render the GPX track on the map
void render_gpx_track(Renderer *r, GpxTrack *track);

#endif // GPX_OVERLAY_H