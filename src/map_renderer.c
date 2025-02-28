#include "map_renderer.h"
#include "stb_ds.h"
#include "tile_req.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>
#include <stdatomic.h>
#include <stdio.h>

#define SCREEN_TILE_COUNT 8 // desired tile count
#define MAX_ZOOM 19
#define MIN_ZOOM 4
#define ZOOM_SPEED 0.15

#define SCREEN_TL(__s) ((Coord){__s.min.x, __s.max.y})
#define SCREEN_BR(__s) ((Coord){__s.max.x, __s.min.y})

#define MAX(_X, _Y) (_X) > (_Y) ? (_X) : (_Y)
#define ZOOM_FROM_SIZE(_W) (int)log2(360 / (_W))

// glob
static atomic_int active_download_threads = 0;

void increment_threads() { atomic_fetch_add(&active_download_threads, 1); }

int get_thread_count() { return active_download_threads; }

void decrement_threads() {
  atomic_fetch_sub(&active_download_threads, 1);
  assert(atomic_load(&active_download_threads) >= 0);
}

Tile coord_to_tile(Coord p, int z) {
  Tile t;
  t.zoom = z;
  t.x = (int)(floor((p.x + 180.0) / 360.0 * (1 << z)));
  double latrad = p.y * M_PI / 180.0;
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
  double lat_rad =
      fmax(-85.0511, fmin(85.0511, p.y)) * M_PI / 180.0; // Clamp latitude
  v.y = scale * (1.0 - asinh(tan(lat_rad)) / M_PI) / 2.0;

  return v;
}

// Function to convert Mercator pixel coordinates to longitude and latitude
Coord pixel_space_to_coord(Vector2 p, int zoom) {
  Coord c;

  int scale = 256 * (1 << zoom);
  // Normalize pixel coordinates to [0, 1]
  double mercator_x = (double)p.x / scale;
  double mercator_y = (double)p.y / scale;
  // Convert to longitude
  c.x = mercator_x * 360.0 - 180.0;
  // Convert to latitude using inverse Mercator formula
  c.y = atan(sinh(M_PI * (1.0 - 2.0 * mercator_y))) * (180.0 / M_PI);
  return c;
}

MapBB get_rect_bb(Coord top_left, double width, double height) {
  MapBB b;
  b.min.x = top_left.x;
  b.max.y = top_left.y;
  b.max.x = top_left.x + width;
  b.min.y = top_left.y - height;
  return b;
}

Vector2 coord_to_screen(Renderer *r, Coord p) {
  Vector2 pixel_topleft = coord_to_pixel_space(SCREEN_TL(r->screen), r->zoom);
  Vector2 pixel_bottomright =
      coord_to_pixel_space(SCREEN_BR(r->screen), r->zoom);

  double scale = fmax(r->width / (pixel_bottomright.x - pixel_topleft.x),
                      r->height / (pixel_bottomright.y - pixel_topleft.y));

  Vector2 pixel_coord = coord_to_pixel_space(p, r->zoom);

  float x_screen = (pixel_coord.x - pixel_topleft.x) * scale;
  float y_screen = (pixel_coord.y - pixel_topleft.y) * scale;

  return (Vector2){
      .x = x_screen,
      .y = y_screen,
  };
}

Coord screen_to_coord(Renderer *r, Vector2 screen_pos) {

  Vector2 pixel_tl = coord_to_pixel_space(SCREEN_TL(r->screen), r->zoom);
  Vector2 pixel_br = coord_to_pixel_space(SCREEN_BR(r->screen), r->zoom);
  double scale = fmax(r->width / (pixel_br.x - pixel_tl.x),
                      r->height / (pixel_br.y - pixel_tl.y));

  Vector2 pixel_pos = {
      .x = pixel_tl.x + screen_pos.x / scale,
      .y = pixel_tl.y + screen_pos.y / scale,
  };

  return pixel_space_to_coord(pixel_pos, r->zoom);
}

Rectangle tile_screen_rect(Renderer *r, Tile t) {
  // mercator pixel_coords
  Vector2 pixel_topleft = coord_to_pixel_space(SCREEN_TL(r->screen), t.zoom);
  Vector2 pixel_bottomright =
      coord_to_pixel_space(SCREEN_BR(r->screen), t.zoom);

  double scale = fmax(r->width / (pixel_bottomright.x - pixel_topleft.x),
                      r->height / (pixel_bottomright.y - pixel_topleft.y));

  double tile_pixel_x = t.x * 256;
  double tile_pixel_y = t.y * 256;

  float tile_x_screen = (tile_pixel_x - pixel_topleft.x) * scale;
  float tile_y_screen = (tile_pixel_y - pixel_topleft.y) * scale;
  float tile_width = 256 * scale;
  float tile_height = 256 * scale;

  return (Rectangle){tile_x_screen, tile_y_screen, tile_width, tile_height};
}

void init_map_renderer(Renderer *r, MapBB screen, int width, int height) {
  r->width = width;
  r->height = height;
  r->screen = screen;
  r->zoom = ZOOM_FROM_WIDTH((screen.max.x - screen.min.x) / SCREEN_TILE_COUNT);
  r->tile_cache = NULL;
  pthread_mutex_init(&r->mutex, NULL);

  InitWindow(r->width, r->height, "Jpx");
  SetTargetFPS(144);
}

void deinit_map_renderer(Renderer *r) {
  pthread_mutex_destroy(&r->mutex);

  // Unload cached tiles
  for (int i = 0; i < hmlen(r->tile_cache); i++) {
    TileData data = r->tile_cache[i].value;
    if (data.status == TILE_READY)
      UnloadTexture(data.texture);
    else if (data.status == TILE_LOADED)
      UnloadImage(data.tile_img);
  }
  CloseWindow();
}

#define FALLBACK_LIMIT 4
void render_fallback_tile(Tile t, Renderer *r) {
  // lower res tiles
  for (int i = 0; i < FALLBACK_LIMIT; i++) {
    t.zoom -= 1;
    t.x /= 2;
    t.y /= 2;
    Item *item;

    item = hmgetp_null(r->tile_cache, t);
    if (item == NULL) {
      // item = load_tile_from_file(t, r->tile_cache);
      //  item couldnt be loaded from file
      if (item == NULL)
        continue;
    }

    if (item->value.status == TILE_NOT_READY)
      continue;
    if (item->value.status == TILE_LOADED) {
      item->value.status = TILE_READY;
      item->value.texture = LoadTextureFromImage(item->value.tile_img);
      UnloadImage(item->value.tile_img);
    }
    // found fallback
    item->value.last_accessed = GetTime(); // update last access time
    Texture texture = item->value.texture;
    Rectangle rec = tile_screen_rect(r, t);
    Rectangle src = {0, 0, texture.width, texture.height};
    DrawTexturePro(texture, src, rec, (Vector2){0, 0}, 0, WHITE);
    DrawRectangleLinesEx(rec, 1, DARKGREEN);
    return;
  }
}

void render_tiles(Renderer *r) {

  Tile min = coord_to_tile(SCREEN_TL(r->screen), r->zoom);
  Tile max = coord_to_tile(SCREEN_BR(r->screen), r->zoom);

  Tile request_tiles[MAX_TILES_PER_REQUEST];
  int tile_req_count = 0;

  int cap = (max.x - min.x + 1) * (max.y - min.y + 1);
  Item ready_tiles[cap];
  int ready_tile_count = 0;

  int unloaded = 0;
  for (int y = min.y; y <= max.y; y++) {
    for (int x = min.x; x <= max.x; x++) {
      Tile t = {r->zoom, x, y};
      bool fallback = false;

      Item *item = hmgetp_null(r->tile_cache, t);
      if (item == NULL) {
        // not cached
        if (tile_req_count < MAX_TILES_PER_REQUEST)
          request_tiles[tile_req_count++] = t;
        fallback = true;
      } else if (item->value.status == TILE_NOT_READY) {
        unloaded++;
        fallback = true;
      } else if (item->value.status == TILE_LOADED) {
        item->value.status = TILE_READY;
        item->value.texture = LoadTextureFromImage(item->value.tile_img);
        UnloadImage(item->value.tile_img);
      }
      // desired tile is not available
      if (fallback) {
        render_fallback_tile(t, r);
        continue;
      }
      assert(item->value.status == TILE_READY);
      item->value.last_accessed = GetTime(); // update last access time
      ready_tiles[ready_tile_count++] = *item;
    }
  }
  // Render ready tiles after all fallback tiles
  for (int i = 0; i < ready_tile_count; i++) {
    Item item = ready_tiles[i];
    Texture texture = item.value.texture;
    Rectangle rec = tile_screen_rect(r, item.key);
    Rectangle src = {0, 0, texture.width, texture.height};
    DrawTexturePro(texture, src, rec, (Vector2){0, 0}, 0, WHITE);
     DrawRectangleLinesEx(rec, 1, DARKGREEN);
  }

  if (atomic_load(&active_download_threads) < MAX_DL_THREADS &&
      tile_req_count > 0) {
    TileRequest request = {
        .tiles = request_tiles,
        .tile_count = tile_req_count,
    };
    increment_threads();
    fetch_tiles(request, &r->tile_cache, &r->mutex, decrement_threads);
  }
}

void move_screen(Renderer *r) {

  //---Zooming---
  float wheel_dir = 0;
  if (((wheel_dir = GetMouseWheelMove()) > 0 && r->zoom < MAX_ZOOM) ||
      (wheel_dir < 0 && r->zoom > MIN_ZOOM)) {

    double width = r->screen.max.x - r->screen.min.x;
    double height = r->screen.max.y - r->screen.min.y;
    if (wheel_dir > 0) {
      width /= 1 + ZOOM_SPEED;
      height /= 1 + ZOOM_SPEED;
    } else {
      double w, h;
      w = width * (1 + ZOOM_SPEED);
      h = height * (1 + ZOOM_SPEED);
      bool hit_top = r->screen.min.y + h > 85.0511;
      bool hit_bot = r->screen.max.y - h < -85.0511;
      if (r->screen.min.x + w < 180 &&
          (r->screen.min.y + h < 0 ? !hit_top : !hit_bot)) {
        width = w;
        height = h;
      }
    }
    Vector2 mouse_pos = GetMousePosition();
    Coord mouse_coord = screen_to_coord(r, mouse_pos);
    if (r->screen.min.y + height < 0) {
      r->screen.max.y = r->screen.min.y + height;
    } else {
      r->screen.min.y = r->screen.max.y - height;
    }
    r->screen.max.x = r->screen.min.x + width;
    r->zoom = ZOOM_FROM_WIDTH(width / SCREEN_TILE_COUNT);
    Vector2 screen_pos = coord_to_screen(r, mouse_coord);

    Coord coord_delta;
    coord_delta.x =
        screen_to_coord(r, screen_pos).x - screen_to_coord(r, mouse_pos).x;
    coord_delta.y =
        screen_to_coord(r, screen_pos).y - screen_to_coord(r, mouse_pos).y;

    // clamp screen
    if (r->screen.min.x + coord_delta.x > -180 &&
        r->screen.max.x + coord_delta.x < 180) {
      r->screen.min.x += coord_delta.x;
      r->screen.max.x += coord_delta.x;
    }
    if (r->screen.min.y + coord_delta.y > -85.0511 &&
        r->screen.max.y + coord_delta.y < 85.0511) {
      r->screen.min.y += coord_delta.y;
      r->screen.max.y += coord_delta.y;
    }
  }

  // Movement
  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);

    Vector2 curr_mouse_pos = GetMousePosition();
    Vector2 prev_mouse_pos = Vector2Subtract(curr_mouse_pos, GetMouseDelta());

    Coord coord_delta;
    coord_delta.x = screen_to_coord(r, curr_mouse_pos).x -
                    screen_to_coord(r, prev_mouse_pos).x;
    coord_delta.y = screen_to_coord(r, curr_mouse_pos).y -
                    screen_to_coord(r, prev_mouse_pos).y;

    // clamp screen
    if (r->screen.min.x - coord_delta.x > -180 &&
        r->screen.max.x - coord_delta.x < 180) {
      r->screen.min.x -= coord_delta.x;
      r->screen.max.x -= coord_delta.x;
    }
    if (r->screen.min.y - coord_delta.y > -85.0511 &&
        r->screen.max.y - coord_delta.y < 85.0511) {
      r->screen.min.y -= coord_delta.y;
      r->screen.max.y -= coord_delta.y;
    }

  } else {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }
}
