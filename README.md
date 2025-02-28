## JPX MAP

Renders world map using slippy map tiles for GPX file viewing with Strava run overlay support.

## Features
- Interactive world map using OpenStreetMap tiles
- GPX file overlay for displaying tracks (e.g., Strava runs)
- Panning and zooming capabilities
- Automatic centering on GPX track when loaded

## Usage
```
./jpx [path_to_gpx_file]
```

If no GPX file is specified, it will try to load "Afternoon_Run.gpx" from the current directory.

## Controls
- Mouse drag: Pan the map
- Mouse wheel: Zoom in/out
- T key: Toggle GPX track visibility

## Libraries
- raylib for rendering
- libcurl for fetching map tiles
- pthreads for multi-threading

