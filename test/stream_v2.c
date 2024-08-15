#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <errno.h>
#include "zmq.h"
#include "stream.h"
#include <cc_array.h>
#include <cc_hashtable.h>

void *context = NULL;
void *socket_stream = NULL;
/*
    Board = {
        "192.168.0.6": [ch0, ch1, ch2],
        "192.168.0.7": [ch0, ch1, ch2],
        "192.168.0.8": [ch0, ch1, ch2],
    }
*/
CC_HashTable *Board = NULL;

/*
    Conf = {
        123123: ["192.168.0.6", "192.168.0.7", "192.168.0.8"],
        456456: ["192.168.0.9", "192.168.0.10", "192.168.0.11"],
        789789: ["192.168.0.12", "192.168.0.13", "192.168.0.14"],
    }
*/
CC_HashTable *Conf = NULL;
    


int rst()
{
    // channel_control_free(channel_control);
    int rc = 0;


    if(socket_stream!=NULL){
        rc = zmq_close(socket_stream);
        if (rc != 0){
            return rc;
        }
        socket_stream = NULL;
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


    
    return rc;
}

int init(const char *config){
    // config = "192.168.0.6/ai0:7"
    
    if (context == NULL) {
        context = zmq_ctx_new();
    }

    if (socket_stream == NULL) {
        socket_stream = zmq_socket(context, ZMQ_STREAM);
    }
    int rc = 0;  
    rc = socket_timeout_set(socket_stream, 3000);
    if (rc < 0) {
        // printf("status:%d\n", rc);
        printf("Error set timeout\n");
        return -1;
    }

    rc = socket_linger_set(socket_stream, 0);
    if (rc < 0) {
        // printf("status:%d\n", rc);
        printf("Error set linger\n");
        return -2;
    }

    int config_count = parse_config_count(config);
    if (config_count < 0){
        return -3;
    }

    ParseConfig *parse_config = (ParseConfig *)malloc(sizeof(ParseConfig)*config_count);
    parse_config_init(parse_config, config_count);

    board_count = parse_config_data(config, parse_config, config_count);
    if (board_count < 0) {
        return -4;
    }

    rc = board_all_init(parse_config, config_count);
    if (rc < 0) {
        return -5;
    }

    board_check_isall();

    rc = board_all_connect();
    if(rc < 0){
        return -6;
    }
    
    // trig_mode
    board_config_set();

    board_channels_connect();

    return 0;
}

int get_channel_count()
{
    int channel_count = 0;
    for (int i = 0; i < board_count; i++) {
        channel_count += board[i].channel_count;
    }
    return channel_count;
}

int sample_rate(int sample_rate){
    for (int i = 0; i < board_count; i++){
        char msg[256] = {0};
        sprintf(msg, ">sample_rate(%d)\r\n", sample_rate);
        int rc = board_cmd_set(board[i].channel_control, msg);
        if (rc < 0){
            printf("Error board sample rate\n");
            return -1;
        }        
    }
    return 0;
}

int sample_enable(int enable){
    int rc = 0;
    for (int i = 0; i < board_count; i++){   
        if (i == 0){
            // sprintf(msg, ">trig_mode(%d)\r\n", enable);
            board_cmd_set(board[i].channel_control, ">trig_mode(1,0)\r\n");
        }else{
            // sprintf(msg, ">trig_mode(%d)\r\n", enable);
            board_cmd_set(board[i].channel_control, ">trig_mode(0,1)\r\n");
        }
        char msg[256] = {0};
        sprintf(msg, ">sample_enable(%d)\r\n", enable);
        rc = board_cmd_set(board[i].channel_control, msg);
        if (rc < 0){
            printf("Error board sample enable\n");
            return -1;
        }
    }
    return 0;
}

int read(void *data,  int data_size, int channel_size)
{
    size_t copy_size = sizeof(int)*channel_size;
    int channel_count = get_channel_count();
    if (channel_count*channel_size > data_size){
        return -1;
    }
    channels_buffer_realloc(copy_size);
    if(channels_isconnected() < 0){
        return -2;
    }
    int rc = channels_read();
    if (rc < 0){
        // printf("Error reading from server\n");
        return -3;
    }
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            for (int j = 0; j < channel_size; j++) {
                
                // unsigned int raw = (uint32_t)(channels[i].buffer[j]);
                int index = j*4;
                void * ptr = (void *)(ch->buffer+index);
                float *data_ptr = (float*)data+i*channel_size+j;
                int tmp = (int)(convert_to_big_endian(ptr)+1);
                // *data_ptr = (float)(tmp)/0x7fffff * 4.096 * channels[i].gain_value;
            }
        }
    }
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

int parse_config_count(const char* config)
{
    char* line;
    char* context1;
    int count = 0;
    int len_read = 0;
    char* config_copy = strdup(config);
    if (config_copy == NULL) {
        return -1; // Memory allocation error
    }
    line = strtok_s(config_copy, "\n", &context1);  
    while (line != NULL) {
        count++;
        // Get the next line
        line = strtok_s(NULL, "\n", &context1);          
    }
    free(config_copy);
    return count;  
}

int parse_config_init(ParseConfig *p_config, int size)
{
    for (int i = 0; i < size; i++) {
        p_config[i].ip = (char*)calloc(sizeof(char), 128);
        p_config[i].channel_id = -1;
        p_config[i].ctrl = -1;
        p_config[i].gain = -1;
        p_config[i].iepe = -1;
        p_config[i].coupling = -1;
    }
    return 0;
}

int parse_config_data(const char* config, ParseConfig *p_config, int size) 
{
    char* line;
    char* context1;
    int index = 0;
    int channel_id = 0;
    int board_count = 1;

    // Duplicate the string to avoid modifying the original config
    char* config_copy = strdup(config);
    if (config_copy == NULL) {
        return -1; // Memory allocation error
    }

    line = strtok_s(config_copy, "\n", &context1);
    while (line != NULL) {
        if (index >= size){
            free(config_copy);
            return -2; // Too many channels
        }
        char* token;
        char* context2;

        // Extract device address and the rest of the parameters
        token = strtok_s(line, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -3; // Error parsing
        }
        memccpy(p_config[index].ip, token, '\0', 128);
        if(index > 0 && strcmp(p_config[index].ip, p_config[index-1].ip) != 0){
            board_count++;
        }

        token = strtok_s(NULL, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -4; // Error parsing
        }

        // Extract channel number
        if (sscanf(token, "ai%d", &channel_id) != 1) {
            free(config_copy);
            return -5; // Error parsing
        }
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

        p_config[index].channel_id = channel_id;
        if(ctrl == -1){
            return -6;
        }
        p_config[index].ctrl = ctrl;
        p_config[index].gain = gain;
        p_config[index].iepe = iepe;
        p_config[index].coupling = coupling;
        // Store the parsed information into the channels array

        index++;

        // Get the next line
        line = strtok_s(NULL, "\n", &context1);
    }

    free(config_copy);
    return board_count;
}

int get_value(const char* param, const char* key) 
{
    const char* p = strstr(param, key);
    if (p != NULL) {
        return atoi(p + strlen(key));
    }
    return 0; // Default value if key is not found
}

int channel_control_init(Channel *channel_control)
{
    // channel_control = (Channel*)malloc(sizeof(Channel));
    channel_init(channel_control);
    channel_control->id = -1;
    return 0;
}

int board_all_init(ParseConfig *p_config, int size)
{
    // init all board
    for(int i = 0; i < board_count; i++){
        board[i].id = i;
        board[i].ip = (char*)calloc(sizeof(char), 128);
        board[i].channel_control = (Channel*)malloc(sizeof(Channel));
        channel_control_init(board[i].channel_control);
        board[i].channel_count = 0;
        board[i].channels = NULL;
    }

    // get board ip and board channel count 
    int board_index = 0;
    for(int i = 0; i < size; i++){
        if (i == 0){
            memccpy(board[board_index].ip, p_config[i].ip, '\0', 128);
        }
        if (i > 0 && strcmp(p_config[i].ip, board[board_index].ip) != 0){
            board_index++;
            memccpy(board[board_index].ip, p_config[i].ip, '\0', 128);           
        }
        board[board_index].channel_count++;
    }
    for(int i = 0; i < board_count; i++){
        board[i].channels = (Channel*)malloc(sizeof(Channel) * board[i].channel_count);
    }

    // init all board channel, get board channel information 
    int channel_index = 0;
    board_index = 0;
    for(int i = 0; i < size; i++){
        channel_init(&board[board_index].channels[channel_index]);
        board[board_index].channels[channel_index].id = p_config[i].channel_id;
        board[board_index].channels[channel_index].ctrl = p_config[i].ctrl;
        board[board_index].channels[channel_index].gain = p_config[i].gain;
        board[board_index].channels[channel_index].iepe = p_config[i].iepe;
        board[board_index].channels[channel_index].coupling = p_config[i].coupling;
        channel_index++;
        if(channel_index == board[board_index].channel_count){
            channel_index = 0;
            board_index++;
        }
        if(board_index > board_count){
            printf("Error board index too many!!!\n");
            return -1;
        }
    }
    return 0;
}

int channel_init(Channel *channel)
{
    channel->id = 0;
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

int board_check_isall()
{
    for(int i = 0; i < board_count; i++){
        if(board[i].channels[0].id == 6){
            board_channels_realloc(board+i);
        }
    }
    return 0;
}

int board_channels_realloc(Board *board)
{
    int ctrl = board[0].channels[0].ctrl;
    int iepe = board[0].channels[0].iepe;
    int coupling = board[0].channels[0].coupling;
    int gain = board[0].channels[0].gain;
    board->channel_count = 6;
    channel_free(board->channels);
    board->channels = (Channel *)realloc(board->channels, board->channel_count*sizeof(Channel));

    // printf("iepe: %d coupling: %d gain: %d ip: %s\n", iepe, coupling, gain, ip);
    for(int i = 0; i < board->channel_count; i++){
        Channel *ch = board->channels + i;
        channel_init(ch);
        // printf("Channel %d initializing\n", ch->channel_id);
        ch->id = i;
        ch->ctrl = ctrl;
        ch->iepe = iepe;
        ch->coupling = coupling;
        ch->gain = gain;
        // printf("Channel %d initialized\n", ch->id);
    }
    return 0;
}

int board_all_connect()
{
    int rc = 0;
    for(int i = 0;i< board_count;i++){
        rc = channel_connect(board->channel_control, board->ip);    
        if (rc < 0){
            printf("Error board %d connect!\n", board->id);
            return rc;
        }    
    }
    return 0;
}

int channel_connect(Channel *channel, const char *ip) 
{
    char addr[128] = {0};
    int rc = sprintf(addr, "tcp://%s:%d", ip, channel->id+7602);
    if (rc < 0) {
        printf("Error addr syntax%d\n", channel->id);
        return -1;
    }
    // printf("addr: %s\n", addr);
    rc = zmq_connect(socket_stream, addr);
    if (rc != 0) {
        printf("Error connecting Channel: %d\n", channel->id);
        return -2;
    }

    rc = zmq_getsockopt(socket_stream, ZMQ_ROUTING_ID, channel->stream_id, &channel->stream_id_size);
    if (rc != 0) {
        printf("Error channel %d getting socket id option\n", channel->id);
        return -3;
    }
    // data_show(channel->stream_id, channel->stream_id_size);
    rc = channel_read(channel);
    if (rc < 0){
        printf("Error channel %d connect!\n", channel->id);
        return -4;
    }
    channel->isconnected = 1;
    printf("stream id:");
    data_show(channel->stream_id, channel->stream_id_size);
    printf("addr: %s Channel %d is connected\n", addr, channel->id);
    return 0;
}

int board_config_set()
{
    int rc = 0;
    for (int i = 0; i < board_count; i++){
        Channel *ch_c = board[i].channel_control;
        board_rst(ch_c);

        for(int i = 0; i < board[i].channel_count; i++){
            Channel *ch = board[i].channels + i;
            rc = board_channel_ctrl(ch_c, ch);
            rc = board_gain(ch_c, ch);
            rc = board_gain_read(ch_c, ch);
            rc = board_coupling(ch_c, ch);
            rc = board_iepe(ch_c, ch);

        }     
    }
    if (rc < 0){
        printf("Error board init\n");
        return -1;
    }
    return 0;
}

int board_channels_connect()
{
    int rc = 0;
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            rc = channel_connect(ch, b->ip);
            if (rc < 0){
                printf("Error board %d channel %d connect!\n", b->id, ch->id);
                return -1;
            }
        }
    }
    return 0;
}

int channels_buffer_realloc(size_t copy_size)
{
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            if (ch->buffer_size != copy_size){
                ch->buffer = (uint8_t*)realloc(ch->buffer, copy_size);
                memset(ch->buffer, 0, copy_size);
                ch->buffer_size = copy_size;         
            }
        }
    }
    return 0;
}

int channels_isconnected()
{
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            if (ch->isconnected != 1){
                printf("Error channel %d disconnected\n", ch->id);
                return -1;
            }
        }
    }
    return 0;
}

int channels_readcount_rst()
{
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            ch->read_count = 0;
        }
    }
    return 0;
}

int channels_read_finish()
{
    int finish = 0;    
    for (int i = 0; i < board_count; i++){
        Board *b = board + i;
        for (int j = 0; j < b->channel_count; j++){
            Channel *ch = b->channels + j;
            if(ch->ctrl == 0){
                finish++;
            }
            if(ch->read_count == ch->buffer_size){
                finish++;
            }
        }
    }
    int channel_count = get_channel_count();
    if(finish != channel_count){
        return -1;
    }
    return 0;
}

int channel_close(Channel *channel)
{
    zmq_send(socket_stream, channel->stream_id, channel->stream_id_size, ZMQ_SNDMORE);
    zmq_send(socket_stream, NULL, 0, 0);
    return 0;
}

int channel_send(Channel *channel, const char *data) 
{
    zmq_send(socket_stream, channel->stream_id, channel->stream_id_size, ZMQ_SNDMORE);
    zmq_send(socket_stream, data, strlen(data), 0);
    return 0;
}

int channel_read(Channel *channel)
{
    uint8_t id[256] = {0};
    int data_size = 0;
    int id_len = read_stream_id(id, 256);
    if (id_len == -2) {
        printf("channel %d read stream id timeout!\n", channel->id);
        return -1;
    }
    memset(channel->buffer, 0, channel->buffer_size);
    data_size = read_data(channel->buffer, channel->buffer_size);
    // data_show(channel->buffer, data_size);
    if(data_size == -2){
        printf("channel %d read data timeout!\n", channel->id);
        return -2;
    }   
    if(cmp_stream_id(channel->stream_id, id, id_len) != 0){   
        // printf("channel %d match\n", channel->channel_id);
        return -3;
    }     
    
    return data_size;
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

int recv_more()
{
    int recv_more = 0;
    size_t recv_more_size = sizeof(recv_more);
    int rc = zmq_getsockopt(socket_stream, ZMQ_RCVMORE, &recv_more, &recv_more_size);
    if (rc != 0) {
        printf("Error getting socket option\n");
        return -1;
    }
    return recv_more;
}

int read_stream_id(uint8_t *id, int id_size)
{
    if(recv_more() != 0){
        // printf("Error read stream id \n");
        return -1;
    }
    int size = zmq_recv(socket_stream, id, id_size, 0);
    if(size < 0){
        // printf("Error receiving stream id timeout\n");
        return -2;
    }
    return size;
}

int read_data(uint8_t *data, int data_size)
{
    if(recv_more() != 1){
        printf("Error read data \n");
        return -1;
    }
    int size = zmq_recv(socket_stream, data, data_size, 0);
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

int board_rst(Channel* ch_c)
{   
    char msg = ">RST()\r\n";
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){
        printf("Error board reset\n");
        return -1;
    }
    return 0;
}

int board_channel_ctrl(Channel *ch_c, Channel *ch)
{
    char msg[256] = {0};
    sprintf(msg, ">channel_ctrl(%d,%d)\r\n", ch->id, ch->ctrl);
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){      
        printf("Error board channel ctrl: %d\n", rc);
        return -1;
    }
    return 0;
}

int board_gain(Channel *ch_c, Channel *ch)
{
    if(ch->gain == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">gain_sel(%d,%d)\r\n", ch->id, ch->gain);
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){
        printf("Error board gain\n");
        return -1;
    }
    return 0;
}

int board_gain_read(Channel *ch_c, Channel *ch)
{
    char msg[256] = {0};
    sprintf(msg, ">gain_read(%d)\r\n", ch->id);
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){
        printf("Error board gain read\n");
        return -1;
    }
    // printf("buf: %s\n", channel_control->buffer);
    sscanf(ch_c->buffer, "<gain_read(%d) gain=%f, OK!", &(ch->id), &(ch->gain_value));
    printf("gain_value: %f\n", ch->gain_value);
    return 0;   
}

int board_coupling(Channel *ch_c, Channel *ch)
{
    if (ch->coupling == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">coupling_sel(%d,%d)\r\n", ch->id, ch->coupling);
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){
        printf("Error board coupling\n");
        return -1;
    }
    return 0;
}

int board_iepe(Channel *ch_c, Channel *ch)
{
    if (ch->iepe == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">iepe_sel(%d,%d)\r\n", ch->id, ch->iepe);
    int rc = board_cmd_set(ch_c, msg);
    if (rc < 0){
        printf("Error board iepe\n");
        return -1;
    }
    return 0;
}

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

int board_cmd_set(Channel *channel_control, const char *data)
{
    if (channel_control == NULL){
        printf("Error channel control is NULL\n");
        return -1;
    }
    printf("Channel %d send: %s\n", channel_control->id, data);
    channel_send(channel_control, data);
    char *param_name = extract_parameter_name(data);
    // printf("param: %s\n", param_name);
    int rc = channel_read(channel_control);
    if (rc < 0){
        printf("Error board receiving data\n");
        return -2;
    }
    printf("Channel %d recv: %s\n", channel_control->id, channel_control->buffer);
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

int channel_buffer_write(Channel *channel, uint8_t *data, int data_size)
{
    if (channel->read_count + data_size < channel->buffer_size){
        memcpy(channel->buffer + channel->read_count, data, data_size);
        channel->read_count += data_size; 
    }else{
        memcpy(channel->buffer + channel->read_count, data, channel->buffer_size-channel->read_count);
        channel->_dataread_count = channel->buffer_size;
    }   
    return 0;
}

int channels_read()
{
    channels_readcount_rst();
    uint8_t *data = (uint8_t *)calloc(sizeof(uint8_t), board->channels->buffer_size);
    while(1){
        if (channels_read_finish() == 0){
            break;
        }
        char id[256] = {0};
        int rc = read_stream_id(id, 256);
        if (rc <= 0){
            printf("Error read stream id!: %d\n", rc);
            return -1;
        }
        //////////////////////////////////////////////////////
        for (int i = 0; i < board_count; i++){
            Board *b = board + i;
            for (int j = 0; j < b->channel_count; j++){
                Channel *ch = b->channels + j;
                if(cmp_stream_id(id, ch->stream_id, ch->stream_id_size) != 0){
                    // printf("i:%d\n", i);
                    if(i == b->channel_count - 1){
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
    }  
    free(data);
    return 0;  
}


