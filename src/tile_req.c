#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include "stb_ds.h"
#include "tile_req.h"

#define FORMAT ".png"
#define URL "https://tile.openstreetmap.org/%d/%d/%d%s"
//#define TILE_SET "mapbox.satellite"
//"https://api.mapbox.com/v4/%s/%d/%d/%d.jpg?access_token=%s"

typedef struct {
    Tile tile;
    unsigned char *memory;
    size_t size;
} TileChunk;

size_t write_memory(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    TileChunk *chunk = (TileChunk *)userp;
 
    unsigned char *ptr = realloc(chunk->memory, chunk->size + realsize + 1);
    if(!ptr) {
        printf("realloc returned NULL)\n");
        return 0;
    }

    chunk->memory = ptr;
    memcpy(&(chunk->memory[chunk->size]), contents, realsize);
    chunk->size += realsize;
     
    return realsize;
}

void *downloader_thread_func(void *arg) {
    DownloadContext *context = (DownloadContext *)arg;
    TileRequest request = context->request;

    CURLM *multi_handle = curl_multi_init();
    CURL *easy_handles[request.tile_count];
    int num_handles = 0;
    int still_running = 0;

    // Add tiles to the multi handle
    for (int i = 0; i < request.tile_count; i++) {
            char url[sizeof(URL) + 128];
            Tile t = request.tiles[i];
            sprintf(url, URL, t.zoom, t.x, t.y, FORMAT);

            CURL *e_handle = curl_easy_init();
            TileChunk *chunk = malloc(sizeof(TileChunk));
            chunk->memory = malloc(1);
            chunk->size = 0;
            chunk->tile = t;

            curl_easy_setopt(e_handle, CURLOPT_URL, url);
            curl_easy_setopt(e_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
            curl_easy_setopt(e_handle, CURLOPT_WRITEDATA, chunk);
            curl_easy_setopt(e_handle, CURLOPT_WRITEFUNCTION, write_memory);
            curl_easy_setopt(e_handle, CURLOPT_PRIVATE, chunk);

            curl_multi_add_handle(multi_handle, e_handle);
            easy_handles[num_handles++] = e_handle; // track handle
    }

    // perform downloads
    do {
        curl_multi_perform(multi_handle, &still_running);

        // whait for activity or timeout
        curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);

        // check for complete
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                // handle complete
                CURL *easy_handle = msg->easy_handle;

                TileChunk *chunk;
                curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &chunk);

                // Got the data
                if (msg->data.result == CURLE_OK) {

                    Image img = LoadImageFromMemory(FORMAT, chunk->memory, chunk->size);
                    // update cache entry
                    TileData data = {
                        .texture = (Texture){0},
                        .status = TILE_LOADED,
                        .last_accessed = 0,
                    };
                    hmput(*context->tile_cache, chunk->tile, data);
                    // TODO: save to disk

                } else {
                    free(chunk->memory); // free memory on fail
                    fprintf(stderr, "ERROR: Tile download failed\n");
                }
                
                free(chunk);
                curl_multi_remove_handle(multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);
            }
        }
        
    } while(still_running > 0);

    // Complete
    //printf("request for %d tiles complete\n", request.tile_count);
    // decrease passed in active threads value
    int *active_threads = (int *)context->user_data;
    *active_threads = *active_threads > 0 ? *active_threads - 1 : *active_threads;

    // Clean up
    curl_multi_cleanup(multi_handle);

    free(request.tiles);
    free(context);
    return NULL;
}

void start_download_thread(TileRequest request, Item **tile_cache, void *userdata) {
    pthread_t download_thread;
    DownloadContext *context = malloc(sizeof(DownloadContext));

    // allocate the requested array of tiles
    Tile *tiles = malloc(request.tile_count * sizeof(Tile));
    for (int i = 0; i < request.tile_count; i++) {
        Tile t = request.tiles[i];
        tiles[i] = t;
        // add them as cache entry
        TileData data = {
            .tile_img = (Image){0},
            .ready = false,
        };
        hmput(*tile_cache, t, data);
    }
    TileRequest req = {
        .tiles = tiles,
        .tile_count = request.tile_count,
    };

    context->request = req;
    context->user_data = userdata;
    context->tile_cache = tile_cache;

    pthread_create(&download_thread, NULL, downloader_thread_func, context);
    pthread_detach(download_thread);
}
