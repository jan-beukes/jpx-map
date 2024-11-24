#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define URL "https://api.mapbox.com/v4/%s/%d/%d/%d.jpg?access_token=%s"
#define TILE_SET "mapbox.satellite"

size_t write_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    size_t bytes = size * nmemb;
    printf("Wrote %zu bytes\n", bytes);
    return written;
}

void fetch_map_tile(int x, int y, int zoom, const char *savedir) {
#ifndef API_TOK
    #define API_TOK ""
    fprintf(stderr, "Error: No API_TOK defined\n");
    exit(1);
#endif
    CURL *curl;
    CURLcode res;
    FILE *file;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    // Set the URL of the tile
    char url[strlen(URL) + strlen(API_TOK) + 64];
    sprintf(url, URL, TILE_SET, zoom, x, y, API_TOK);
    printf("Request: %s\n", url);

    // Open the file for writing
    char fname[128];
    sprintf(fname, "%s/tile_%dx_%dy.jpg", savedir, x, y);
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

