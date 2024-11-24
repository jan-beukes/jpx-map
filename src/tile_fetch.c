#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "tile_fetch.h"

#ifndef API_TOK
#define API_TOK ""
#endif

#define URL "https://api.mapbox.com/v4/%s/%d/%d/%d/.jpg"
#define TILE_SET "mapbox.satellite"

size_t write_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void fetch_map_tile(int x, int y, int zoom, const char *savedir) {
    CURL *curl;
    CURLcode res;
    FILE *file;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    // Set the URL of the tile
    char url[strlen(URL) + 64];
    sprintf(url, URL, TILE_SET, zoom, x, y);

    // Open the file for writing
    char fname[64];
    sprintf(fname, "tile_%d_%d.jpg", x, y);
    file = fopen(fname, "wb");
    if (!file) {
        fprintf(stderr, "ERROR: couldn't open file %s", fname);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set the write function and pass the file pointer to it
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    fclose(file);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

