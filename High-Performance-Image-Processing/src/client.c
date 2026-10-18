#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<errno.h>
#include "logging.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define IMG_WIDTH 512
#define IMG_HEIGHT 512
#define CHANNELS 4
#define IMG_SIZE (IMG_WIDTH*IMG_HEIGHT*CHANNELS)

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
    {
        log_message(LOG_ERROR, "Usage:  %s <image_filename> [optional_client_id]", argv[0]);
        return 1;
    }
    
    char client_name[32];
    if (argc == 3) {
        snprintf(client_name, sizeof(client_name), "CLIENT_%s", argv[2]);
    } else {
        snprintf(client_name, sizeof(client_name), "CLIENT_%d", getpid());
    }
    log_init(client_name);

    int width, height, bpp;
    unsigned char *original_data = stbi_load(argv[1], &width, &height, &bpp, CHANNELS);

    if(!original_data)
    {
        log_message(LOG_ERROR, "Error loading image. Make sure the file exists.");
        return 1;
    }
    log_message(LOG_INFO, "Original image: %dx%d", width, height);
    unsigned char *resized_data = malloc(IMG_SIZE);
    log_message(LOG_INFO, "Resizing the image to %dx%d..", IMG_WIDTH, IMG_HEIGHT);
    stbir_resize_uint8_linear(original_data, width, height, 0, resized_data, IMG_WIDTH, IMG_HEIGHT, 0, STBIR_RGBA);
    stbi_write_png("debug_resized.png", IMG_WIDTH, IMG_HEIGHT, CHANNELS, resized_data, IMG_WIDTH * CHANNELS);
    
    int sock = 0;
    struct sockaddr_in serv_addr;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        log_message(LOG_ERROR, "Socket creation error!!");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        log_message(LOG_ERROR, "Invalid address or address not supported!!");
        return -1;
    }
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        log_message(LOG_ERROR, "Connection failed!!");
        return -1;
    }
    log_message(LOG_INFO, "Sending %d bytes to the server..", IMG_SIZE);
    send(sock, resized_data, IMG_SIZE, 0);
    unsigned char *received_buffer = malloc(IMG_SIZE);
    
    // Receive Server-Assigned ID first
    int client_id;
    if (recv(sock, &client_id, sizeof(int), 0) <= 0) {
        log_message(LOG_ERROR, "Failed to receive client ID");
        close(sock);
        return 1;
    }
    
    int total_read = 0;
    while(total_read < IMG_SIZE)
    {
        int bytes = recv(sock, received_buffer + total_read, IMG_SIZE - total_read, 0);
        if(bytes <= 0) break;
        total_read = total_read + bytes;
    }
    log_message(LOG_INFO, "Received processed image back from server.");

    char filename[64];
    snprintf(filename, sizeof(filename), "output/output_%d.png", client_id);

    stbi_write_png(filename, IMG_WIDTH, IMG_HEIGHT, CHANNELS, received_buffer, IMG_WIDTH * CHANNELS);
    log_message(LOG_INFO, "Saved output as '%s'", filename);
    stbi_image_free(original_data);
    free(resized_data);
    free(received_buffer);
    close(sock);
    return 0;
}