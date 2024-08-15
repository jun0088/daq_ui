#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <errno.h>
#include "zmq.h"
#include "stream.h"

#define IP_MAX_SIZE 17
Channel *channels = NULL;
Channel *channel_control = NULL;
void *context = NULL;
void *s = NULL;
int channel_count = 0;

int rst()
{
    // channel_control_free(channel_control);
    int rc = 0;
    if (channel_control != NULL){
        channel_free(channel_control);
        free(channel_control);
        channel_control = NULL;        
    }
    if (channels != NULL){
        for (int i = 0; i < channel_count; i++){
            channel_free(channels + i); 
        }
        free(channels);     
        channels = NULL;       
    }

    if(s!=NULL){
        rc = zmq_close(s);
        if (rc != 0){
            return rc;
        }
        s = NULL;
    }

    if (context!=NULL){
        rc =zmq_ctx_shutdown(context);
        if (rc != 0){
            return rc;
        }
        rc =zmq_ctx_term(context);
        if (rc != 0){
            return rc;
        }
        context = NULL; 
    }

    channel_count = 0;
    
    return rc;
}

int init(const char *config, int size){
    // config = "192.168.0.6/ai0:7"
    
    if (context == NULL) {
        context = zmq_ctx_new();
    }

    if (s == NULL) {
        s = zmq_socket(context, ZMQ_STREAM);
    }
    int rc = 0;  
    rc = socket_timeout_set(s, 3000);
    if (rc < 0){
        // printf("status:%d\n", rc);
        printf("Error set timeout\n");
        return -1;
    }

    rc = socket_linger_set(s, 0);
    if (rc < 0){
        // printf("status:%d\n", rc);
        printf("Error set linger\n");
        return -2;
    }

    channel_count = count_crlf(config, size);
    printf("channel count: %d\n", channel_count);
    if (channel_count == 0){
        printf("Error count crlf\n");        
        return -3;
    }

    // printf("Channel count:%d\n", channel_count);    
    channels = (Channel *)malloc(sizeof(Channel)*channel_count);
    channels_init(channels, channel_count);

    rc = parse_config(config, channels);
    printf("parse:%d\n", rc);
    if (rc < 0 || rc != channel_count){
        printf("Error parsing config\n");
        return -4;
    }
    
    char ip[17] = {0};
    strncpy(ip, channels->ip, strlen(channels->ip));
    rc = channel_control_init(ip);
    if (rc < 0){
        printf("Error channel control init\n");
        return -5;
    } 
    // printf("Channel control init\n");

    if (channels->channel_id == 6){
        channel_count = 6;
        channels_realloc();
    }

    // for(int i=0; i<channel_count; i++){
    //     printf("Channel %d: %d\n", i, channels[i].channel_id);
    //     // printf("Channel %d: %d\n", i, channels[i].iepe);
    // }

    rc = board_init(channels, channel_count);
    if (rc < 0){
        printf("Error board init\n");
        return -7;
    }  

    rc = channels_connect(channels, channel_count);
    if (rc < 0){
        printf("Error channels connect\n");
        return -8;
    }
    
    return 0;
}

int get_channel_count()
{
    return channel_count;
}

int sample_rate(int sample_rate){
    char msg[256] = {0};
    sprintf(msg, ">sample_rate(%d)\r\n", sample_rate);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board sample rate\n");
        return -1;
    }
    return 0;
}

int sample_enable(int enable){
    char msg[256] = {0};
    sprintf(msg, ">sample_enable(%d)\r\n", enable);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board sample enable\n");
        return -1;
    }
    return 0;
}

int read(void *data,  int data_size, int channel_size)
{
    size_t copy_size = sizeof(int)*channel_size;
    if (channel_count*channel_size > data_size){
        return -1;
    }
    channels_buffer_realloc(channels, copy_size);
    if(channels_isconnected(channels) < 0){
        return -2;
    }
    int rc = channels_read(s, channels, channel_count);
    if (rc < 0){
        // printf("Error reading from server\n");
        return -3;
    }

    for (int i = 0; i < channel_count; i++){
        // printf("Channel %d data: ", (channels+i)->channel_id);
        // data_show((channels+i)->buffer, copy_size);
        for (int j = 0; j < channel_size; j++) {
            
            // unsigned int raw = (uint32_t)(channels[i].buffer[j]);
            int index = j*4;
            void * ptr = (void *)(channels[i].buffer+index);
            // int raw = *(int *)ptr;
            // int raw = convert_to_big_endian(ptr);
            // printf("ptr: %08x raw: %08x \n", *(uint32_t *)ptr, raw);
            // return 0;
            // int raw24 = (int32_t)(((raw+1)<<8)>>8);
            // float volt = ((float)raw24) / 0x7fffff * 4;
            // float gain = 0;
            // if (channels[i].gain == 1){
            //     gain = 10.0;
            // } else{
            //     gain = 1.0;
            // }
            // float converted = (float)volt / gain;
            // float converted = (float)volt * 1000000 / (float)(1);
            // printf("volt: %f converted: %f\n", volt, converted);
            float *data_ptr = (float*)data+i*channel_size+j;
            int tmp = (int)(convert_to_big_endian(ptr)+1);
            // tmp &= 0xffffff;
            *data_ptr = (float)(tmp)/0x7fffff * 4.096 * channels[i].gain_value;
            // *data_ptr = (float)((*(uint32_t *)ptr+1) & 0xffffff) / 0x7fffff *4;
            // data[i*channel_size+j] = convert_to_big_endian(&converted);
        }
        // memcpy(data + i*channel_size, (channels+i)->buffer, (channels+i)->buffer_size);
        
    }
    // printf("Read from server1111111\n");
    return 0;
}


int socket_timeout_set(void *socket, int timeout)
{
    int rc = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc != 0) {
        printf("Error setting socket timeout\n");
        return -1;
    }
    return 0;
}

int socket_linger_set(void *socket, int linger)
{
    int rc = zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
    if (rc != 0) {
        printf("Error setting socket timeout\n");
        return -1;
    }
    return 0;
}

int channel_init(Channel *channel)
{
    channel->ip = (char*)calloc(sizeof(char), IP_MAX_SIZE);
    channel->channel_id = 0;
    channel->stream_id_size = 256;
    channel->stream_id = (uint8_t*)calloc(sizeof(uint8_t), channel->stream_id_size);
    channel->buffer_size = 1024;
    channel->buffer = (uint8_t*)calloc(sizeof(uint8_t), channel->buffer_size);
    channel->read_count = 0;
    channel->ctrl = 0;
    channel->gain = 0;
    channel->iepe = 0;
    channel->coupling = 0;
    channel->isconnected = 0;
    channel->gain_value = 0.0;
    return 0;
}

int channel_free(Channel *channel)
{
    if(channel->ip != NULL){
        free(channel->ip);
        channel->ip = NULL;
    }
    if (channel->stream_id != NULL) {
        free(channel->stream_id);
        channel->stream_id = NULL;
    }
    if (channel->buffer != NULL) {
        free(channel->buffer);
        channel->buffer = NULL;
    }
    return 0;
    
}

int channels_realloc()
{
    int iepe = channels[0].iepe;
    int coupling = channels[0].coupling;
    int gain = channels[0].gain;
    char *ip = strdup(channels[0].ip);
    
    channel_free(channels);
    channels = (Channel *)realloc(channels, channel_count*sizeof(Channel));
    if (channels == NULL){
        printf("Error allocating memory for channels\n");
        return -1;
    }
    channels_init(channels, channel_count);

    // printf("iepe: %d coupling: %d gain: %d ip: %s\n", iepe, coupling, gain, ip);
    for(int i = 0; i < channel_count; i++){
        Channel *ch = channels + i;
        // printf("Channel %d initializing\n", ch->channel_id);
        ch->channel_id = i;
        ch->ctrl = 1;
        memcpy(ch->ip, ip, strlen(ip));
        ch->iepe = iepe;
        ch->coupling = coupling;
        ch->gain = gain;
        printf("Channel %d initialized\n", ch->channel_id);
    }
    free(ip);
    return 0;
}

int channels_init(Channel *channels, int size)
{
    for (int i = 0; i < size; i++){
        channel_init(channels + i);
    }
    return 0;
}

int channel_buffer_realloc(Channel *channel, int buf_size)
{
    if (channel->buffer_size != buf_size){
        channel->buffer = (uint8_t*)realloc(channel->buffer, buf_size);
        memset(channel->buffer, 0, buf_size);
        channel->buffer_size = buf_size;         
    }
    return 0;
}

int channel_connect(void *socket, Channel *channel) 
{
    char addr[128] = {0};

    int rc = sprintf(addr, "tcp://%s:%d", channel->ip, channel->channel_id+7602);
    if (rc < 0) {
        printf("Error addr syntax%d\n", channel->channel_id);
        return -1;
    }
    // printf("addr: %s\n", addr);
    rc = zmq_connect(socket, addr);
    if (rc != 0) {
        printf("Error connecting Channel: %d\n", channel->channel_id);
        return -2;
    }

    rc = zmq_getsockopt(socket, ZMQ_ROUTING_ID, channel->stream_id, &channel->stream_id_size);
    if (rc != 0) {
        printf("Error channel %d getting socket id option\n", channel->channel_id);
        return -3;
    }
    // data_show(channel->stream_id, channel->stream_id_size);
    rc = channel_read(socket, channel);
    if (rc < 0){
        printf("Error channel %d connect!\n", channel->channel_id);
        return -4;
    }
    channel->isconnected = 1;
    printf("stream id:");
    data_show(channel->stream_id, channel->stream_id_size);
    printf("Channel %d is connected\n", channel->channel_id);
    return 0;
}

int channels_connect(Channel *channels, int size)
{
    for (int i = 0; i < size; i++)
    {   
        int rc = channel_connect(s, channels + i);
        if(rc < 0){
            // printf("status:%d\n", rc);
            printf("Error connecting to server\n"); 
            return -1;
        }        
    } 
    return 0;  
}

int channel_close(void *socket, Channel *channel)
{
    zmq_send(socket, channel->stream_id, channel->stream_id_size, ZMQ_SNDMORE);
    zmq_send(socket, NULL, 0, 0);
    return 0;
}

int channel_send(void *socket, Channel *channel, const char *data) 
{
    zmq_send(socket, channel->stream_id, channel->stream_id_size, ZMQ_SNDMORE);
    zmq_send(socket, data, strlen(data), 0);
    return 0;
}

int channel_read(void *socket, Channel *channel)
{
    uint8_t id[256] = {0};
    int data_size = 0;
    int id_len = read_stream_id(socket, id, 256);
    if (id_len == -2) {
        printf("channel %d read stream id timeout!\n", channel->channel_id);
        return -1;
    }
    memset(channel->buffer, 0, channel->buffer_size);
    data_size = read_data(socket, channel->buffer, channel->buffer_size);
    // data_show(channel->buffer, data_size);
    if(data_size == -2){
        printf("channel %d read data timeout!\n", channel->channel_id);
        return -2;
    }   
    if(cmp_stream_id(channel->stream_id, id, id_len) != 0){   
        // printf("channel %d match\n", channel->channel_id);
        return -3;
    }     
    
    return data_size;
}

int channel_control_init(const char* ip)
{
    channel_control = (Channel*)malloc(sizeof(Channel));
    channel_init(channel_control);
    channel_control->channel_id = -1;
    memcpy(channel_control->ip, ip, strlen(ip));
    // printf("channel ip : %s\n", channel_control->ip);
    int rc = channel_connect(s, channel_control);
    if(rc < 0){
        // printf("Error Channel control connecting to server\n");
        channel_control_free(channel_control); 
        return -1;
    }
    return 0;
}

int channel_control_free(Channel *channel_control)
{
    if (channel_control != NULL){
        channel_free(channel_control);
        free(channel_control);
        channel_control = NULL;        
    }

    return 0;
}

int recv_more(void* socket)
{
    int recv_more = 0;
    size_t recv_more_size = sizeof(recv_more);
    int rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &recv_more, &recv_more_size);
    if (rc != 0) {
        printf("Error getting socket option\n");
        return -1;
    }
    return recv_more;
}

int read_stream_id(void *socket, uint8_t *id, int id_size)
{
    if(recv_more(socket) != 0){
        // printf("Error read stream id \n");
        return -1;
    }
    int size = zmq_recv(socket, id, id_size, 0);
    if(size < 0){
        // printf("Error receiving stream id timeout\n");
        return -2;
    }
    return size;
}

int read_data(void *socket, uint8_t *data, int data_size)
{
    if(recv_more(socket) != 1){
        printf("Error read data \n");
        return -1;
    }
    int size = zmq_recv(socket, data, data_size, 0);
    if(size < 0){
        printf("Error receiving data timeout\n");
        return -2;
    }
    return size;
}

int cmp_stream_id(const void *a, const void *b, size_t size)
{
    uint8_t *id_a = (uint8_t *)a;
    uint8_t *id_b = (uint8_t *)b;
    for (int i = 0; i < size; i++) {
        if (id_a[i] != id_b[i]) {
            return -1;
        }
    }
    return 0;
}

int data_show(uint8_t *data, size_t size)
{
    for (int i = 0; i < size; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
    return 0;
}

int count_crlf(const char *str, int size) 
{  
    char* line;
    char* context1;
    int count = 0;
    char* config_copy = strdup(str);
    if (config_copy == NULL) {
        return -1; // Memory allocation error
    }
    line = strtok_s(config_copy, "\n", &context1);  
    while (line != NULL) {
        if (sscanf(line, "ai%d", &count) == 1) {
            printf("%d\n", count);
            return count;
        }
        count++;
        // Get the next line
        line = strtok_s(NULL, "\n", &context1);          
    }
    free(config_copy);
    return count;  
} 

int get_value(const char* param, const char* key) 
{
    const char* p = strstr(param, key);
    if (p != NULL) {
        return atoi(p + strlen(key));
    }
    return 0; // Default value if key is not found
}

int parse_config(char* config, Channel* channels) 
{
    if (channels == NULL) {
        return -1; // Invalid input
    }
    char* line;
    char* context1;
    int channel_index = 0;
    int channel_id = 0;

    // Duplicate the string to avoid modifying the original config
    char* config_copy = strdup(config);
    if (config_copy == NULL) {
        return -2; // Memory allocation error
    }

    line = strtok_s(config_copy, "\n", &context1);
    while (line != NULL) {
        if (channel_index >= channel_count){
            free(config_copy);
            return -3; // Too many channels
        }
        char* token;
        char* context2;

        // Extract device address and the rest of the parameters
        token = strtok_s(line, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -4; // Error parsing
        }
        memccpy(channels[channel_index].ip, token, '\0', IP_MAX_SIZE);
        // channels[channel_index].ip = strdup(token);
        // char* device_address = token; // e.g., "192.168.17.100"
        token = strtok_s(NULL, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -5; // Error parsing
        }

        // Extract channel number
        if (sscanf(token, "ai%d", &channel_id) != 1) {
            free(config_copy);
            return -6; // Error parsing
        }
        if (channel_index > 0){
            if (channel_id == channels[channel_index - 1].channel_id){
                free(config_copy);
                return -7; // Channels must be consecutive
            }
        }
        // printf("Channel number: %d\n", channel_number);
        // Initialize default values
        int ctrl = -1, gain = -1, iepe = -1, coupling = -1; 

        // Extract parameters
        token = strtok_s(token, "?", &context2);
        if (token != NULL) {
            token = strtok_s(NULL, "&", &context2);
            while (token != NULL) {
                if (strncmp(token, "ctrl=", 5) == 0) {
                    ctrl = get_value(token, "ctrl=");
                } else if (strncmp(token, "gain=", 5) == 0) {
                    gain = get_value(token, "gain=");
                } else if (strncmp(token, "iepe=", 5) == 0) {
                    iepe = get_value(token, "iepe=");
                } else if (strncmp(token, "coupling=", 9) == 0) {
                    coupling = get_value(token, "coupling=");
                }
                token = strtok_s(NULL, "&", &context2);
            }
        }

        channels[channel_index].channel_id = channel_id;
        if(ctrl == -1){
            return -8;
        }
        channels[channel_index].ctrl = ctrl;
        channels[channel_index].gain = gain;
        channels[channel_index].iepe = iepe;
        channels[channel_index].coupling = coupling;
        // Store the parsed information into the channels array

        channel_index++;

        // Get the next line
        line = strtok_s(NULL, "\n", &context1);
    }

    free(config_copy);
    return channel_index; // Return the number of parsed channels
}

// int board_close(Channel *channel_control)
// {
//     channel_close(s, channel_control);
//     return 0;    
// }

// int board_all_init()
// {
//     int rc = board_rst();

//     for (int i = 0; i < 6; i++){
//         rc = board_channel_ctrl(i, 1);
//     }
//     rc = board_gain(channels->channel_id, channels->gain, &channels->gain_value);
//     rc = board_coupling(channels->channel_id, channels->coupling);
//     rc = board_iepe(channels->channel_id, channels->iepe);
//     if (rc < 0){
//         return -1;
//     }
//     return 0;
// }

int board_init(Channel *channels, int channel_count)
{
    int rc = board_rst();
    for(int i = 0; i < channel_count; i++){
        Channel *ch = channels + i;
        rc = board_channel_ctrl(ch->channel_id, ch->ctrl);
        rc = board_gain(ch->channel_id, ch->gain);
        rc = board_gain_read(ch->channel_id, &ch->gain_value);
        rc = board_coupling(ch->channel_id, ch->coupling);
        rc = board_iepe(ch->channel_id, ch->iepe);

    }
    if (rc < 0){
        printf("Error board init\n");
        return -1;
    }
    return 0;
}

// int board_start(int sample_rate, int able)
// {
//     int rc = 0;
//     rc = board_sample_rate(sample_rate);
//     rc = board_sample_enable(able);
//     if (rc < 0){
//         printf("Error board start\n");
//         return -1;
//     }
//     return 0;
// }

int board_rst()
{   
    int rc = board_send_and_read(">RST()\r\n");
    if (rc < 0){
        printf("Error board reset\n");
        return -1;
    }
    return 0;
}

int board_iepe(int ch, int iepe)
{
    if (iepe == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">iepe_sel(%d,%d)\r\n", ch, iepe);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board iepe\n");
        return -1;
    }
    return 0;
}

int board_gain(int ch, int gain)
{
    if(gain == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">gain_sel(%d,%d)\r\n", ch, gain);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board gain\n");
        return -1;
    }
    return 0;
}

int board_gain_read(int ch, float *gain_value)
{
    char msg[256] = {0};
    sprintf(msg, ">gain_read(%d)\r\n", ch);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board gain read\n");
        return -1;
    }
    // printf("buf: %s\n", channel_control->buffer);
    sscanf(channel_control->buffer, "<gain_read(%d) gain=%f, OK!", &ch, gain_value);
    // printf("gain_value: %f\n", *gain_value);
    return 0;   
}

int board_coupling(int ch, int coupling)
{
    if (coupling == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">coupling_sel(%d,%d)\r\n", ch, coupling);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        printf("Error board coupling\n");
        return -1;
    }
    return 0;
}

int board_channel_ctrl(int ch, int channel_ctrl)
{
    char msg[256] = {0};
    sprintf(msg, ">channel_ctrl(%d,%d)\r\n", ch, channel_ctrl);
    int rc = board_send_and_read(msg);
    if (rc < 0){
        
        printf("Error board channel ctrl: %d\n", rc);
        return -1;
    }
    return 0;
}

// int board_sample_rate(int sample_rate)
// {
//     char msg[256] = {0};
//     sprintf(msg, ">sample_rate(%d)\r\n", sample_rate);
//     int rc = board_send_and_read(msg);
//     if (rc < 0){
//         printf("Error board sample rate\n");
//         return -1;
//     }
//     return 0;
// }

// int board_sample_enable(int sample_enable)
// {
//     char msg[256] = {0};
//     sprintf(msg, ">sample_enable(%d)\r\n", sample_enable);
//     int rc = board_send_and_read(msg);
//     if (rc < 0){
//         printf("Error board sample enable\n");
//         return -1;
//     }
//     return 0;
// }

char* extract_parameter_name(char* input_str) 
{
    char* start = strchr(input_str, '>');
    if (start == NULL) {
        return NULL;
    }

    start++;
    char* end = strchr(start, '(');
    if (end == NULL) {
        return NULL;
    }

    int length = end - start;
    char* result = (char*)malloc(length + 1);
    strncpy(result, start, length);
    result[length] = '\0';

    return result;
}

int board_send_and_read(const char *data)
{
    if (channel_control == NULL){
        printf("Error channel control is NULL\n");
        return -1;
    }
    printf("Channel %d send: %s\n", channel_control->channel_id, data);
    channel_send(s, channel_control, data);
    char *param_name = extract_parameter_name(data);
    // printf("param: %s\n", param_name);
    int rc = channel_read(s, channel_control);
    if (rc < 0){
        printf("Error board receiving data\n");
        return -2;
    }
    printf("Channel %d recv: %s\n", channel_control->channel_id, channel_control->buffer);
    if (strstr(channel_control->buffer, "OK") == NULL || strstr(channel_control->buffer, param_name) == NULL){
        return -3;
    }     
    
    // printf("Channel %d recv: %s\n", channel_control->channel_id, channel_control->buffer); 

    return 0;
}

uint32_t convert_to_big_endian(uint8_t* data) 
{
    return (data[0] << 24 | data[1] << 16 | data[2] << 8| data[3]);
}

int channels_buffer_realloc(Channel *channels, size_t copy_size)
{
    for (int i = 0; i < channel_count; i++){
        channel_buffer_realloc(channels + i, copy_size);
    }
    return 0;
}

int channels_isconnected(Channel *channels)
{
    for(int i = 0; i < channel_count; i++){
        if((channels+i)->isconnected != 1){
            printf("Error channel %d disconnected\n", (channels+i)->channel_id);
            return -1;
        }
    }
    return 0;
}

int channels_readcount_rst(Channel *channels, int channel_count)
{
    for (int i = 0; i < channel_count; i++){
        (channels+i)->read_count = 0;
    }
    return 0;
}

int channels_read_finish(Channel *channels, int channel_count)
{
    int finish = 0;
    for (int i = 0; i < channel_count; i++){
        Channel *ch = channels+ i;
        if(ch->ctrl == 0){
            finish++;
        }
        if(ch->read_count == ch->buffer_size){
            finish++;
        }
    }
    if(finish != channel_count){
        return -1;
    }
    return 0;
}

int channel_buffer_write(Channel *channel, uint8_t *data, int data_size)
{
    if (channel->read_count + data_size < channel->buffer_size){
        memcpy(channel->buffer + channel->read_count, data, data_size);
        channel->read_count += data_size; 
    }else{
        memcpy(channel->buffer + channel->read_count, data, channel->buffer_size-channel->read_count);
        channel->read_count = channel->buffer_size;
    }   
    return 0;
}

int channels_read(void *socket, Channel *channels, int channel_count)
{
    channels_readcount_rst(channels, channel_count);
    uint8_t *data = (uint8_t *)calloc(sizeof(uint8_t), channels->buffer_size);
    while(1){
        if (channels_read_finish(channels, channel_count) == 0){
            break;
        }
        char id[256] = {0};
        int rc = read_stream_id(socket, id, 256);
        if (rc <= 0){
            printf("Error read stream id!: %d\n", rc);
            return -1;
        }
        for (int i = 0; i < channel_count; i++){
            Channel *ch = (channels+i);
            if(cmp_stream_id(id, ch->stream_id, ch->stream_id_size) != 0){
                // printf("i:%d\n", i);
                if(i == channel_count - 1){
                    printf("Error read have no match stream id\n");
                    return -2;
                }
                continue;
            }
            int size = read_data(socket, data, ch->buffer_size);
            if (size <= 0){
                printf("Error read data\n");
                return -2;
            } 
            channel_buffer_write(ch, data, size);
            // printf("-------------------\n");
            break;
        }      
    }  
    free(data);
    return 0;  
}


