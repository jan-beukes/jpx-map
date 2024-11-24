#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "image_req.h"

#ifndef API_TOK
#define API_TOK ""
#endif

// callback function for curl to call which handles content
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    MemChunk *mem = (MemChunk *)userp;

    uint8_t *ptr = realloc(mem->memory, mem->size + real_size + 1);
    if (!ptr) {
        printf("ERROR: realloc fail");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

MemChunk fetch_image(const char *url, size_t len) {
    CURL *curl;
    CURLcode res;
    MemChunk chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    // API TOKEN concat
    char buff[len + strlen(API_TOK) + 1];
    strcpy(buff, url);
    strcat(buff, API_TOK);
    curl_easy_setopt(curl, CURLOPT_URL, buff);

    // sends data to callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    // pass chunk struct to callback function
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "ERROR: failed to perform curl: %s", curl_easy_strerror(res));
    } else {
        printf("%lu bytes recieved\n", (unsigned long)chunk.size);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return chunk;
}


