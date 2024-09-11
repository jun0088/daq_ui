#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Windows.h>
#include <time.h>
#include "zmq.h"
#include "daq.h"
#include "cc_array.h"
#include "cc_hashtable.h"
#include "calibrate.h"
int static _lock = 0;

void *context = NULL;
/*
    Board = {
        "192.168.0.6": [ch0, ch1, ch2],
        "192.168.0.7": [ch0, ch1, ch2],
        "192.168.0.8": [ch0, ch1, ch2],
    }
*/
static CC_HashTable *board = NULL;

/*
    Conf = {
        123123: ["192.168.0.6", "192.168.0.7", "192.168.0.8"],
        456456: ["192.168.0.9", "192.168.0.10", "192.168.0.11"],
        789789: ["192.168.0.12", "192.168.0.13", "192.168.0.14"],
    }
*/
static CC_HashTable *conf = NULL;

static HANDLE hMutex = NULL;
int test_num = 0;

int channel_free(Channel *channel);
int arr_ip_free(char *ip);

// int open_exe(char *file)
// {
//     int rc = system(file);
//     if (rc != 0) {
//         printf("open %s failed\n", file);
//         return -1;
//     }
//     return 0;
// }

// int lock()
// {
//     // lock
//     WaitForSingleObject(hMutex,INFINITE);
//     // sleep 5s
//     Sleep(1000);

//     test_num++;
//     // releasse
//     ReleaseMutex(hMutex);
//     return test_num;
// }




/**
 * init global variable: Mutex, zmq context, array board and conf
 */
int init(){
    if (hMutex == NULL) {
        hMutex = CreateMutex(NULL, FALSE, NULL);
    }
    if (context == NULL) {
        context = zmq_ctx_new();
    }
    if (board == NULL) {
        cc_hashtable_new(&board);
    }
    if (conf == NULL) {
        cc_hashtable_new(&conf);    
    }
    return 0;
}

int get_board_conf_count()
{
    // WaitForSingleObject(hMutex,INFINITE);
    int board_count = 0;
    int conf_count = 0;
    if (board != NULL) {
        board_count = cc_hashtable_size(board);
    }
    if (conf != NULL){
        conf_count = cc_hashtable_size(conf);
    }
    // ReleaseMutex(hMutex);
    return board_count*10 + conf_count;
}

int rst()
{
    int rc = 0;     
    if (context != NULL) {
        rc =zmq_ctx_shutdown(context);
        if (rc != 0){
            return -1;
        }
        rc =zmq_ctx_term(context);
        if (rc != 0){
            return -2;
        }
        context = NULL; 
    }               
    return 0;
}

/**
 * init board
 *
 * @param[in] config the config string
 *
 * @return key if the creation was successful, <0 otherwise
 */
int board_init(const char *config)
{
    int rc = 0;

    time_t start_time = time(NULL);
    time_t timeout = 3;

    while (InterlockedCompareExchange(&_lock, 1, 0)){
        Sleep(1000);
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time); 
        if (elapsed_time >= timeout) {
            return -9;
        }       
    }
    rc = init();

    InterlockedCompareExchange(&_lock,0,1);
    if (rc != 0) {
        return -8;
    }
    
    // rc = init();
    WaitForSingleObject(hMutex,INFINITE);
    int key = hash_string(config);
    CC_Array *arr_ip;
    cc_array_new(&arr_ip);
    rc = parse_config(config, arr_ip);
    if (rc < 0) {
        printf("Error parse config: %d\n", rc);
        return -1;
    }
    conf_add(key, arr_ip); 
    rc = board_isall(key);
    if (rc < 0) {
        printf("Error board isall: %d\n", rc);
        return -2;
    }
    
    rc = board_add_control(key);
    if (rc < 0) {
        printf("Error board add control: %d\n", rc);
        return -3;
    }
    rc = board_add_socket_stream(key);
    if (rc < 0) {
        printf("Error board add socket stream: %d\n", rc);
        return -4;
    }
    rc = board_ch_ctrl_connect(key);
    if (rc < 0) {
        printf("Error board connect: %d\n", rc);
        return -5*10;
    }
    rc = board_set_conf(key);
    if (rc < 0) {
        printf("Error board set conf: %d\n", rc);
        return -6;
    }
    rc = board_connect(key);
    if (rc < 0) {
        printf("Error board connect: %d\n", rc);
        return -7;
    }
    ReleaseMutex(hMutex);
    return key;
}

/**
 * free board
 *
 * @param[in] key the key of a group of boards
 *
 * @return 0 if the free was successful, <0 otherwise
 */
int board_free(const int key)
{

    // int static _lock = 0;

    WaitForSingleObject(hMutex,INFINITE);
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key get arr_ip: %d\n", key);
        return -1;
    }
    // int size = cc_array_size(arr_ip);
    // printf("size:%d\n", size);
    int socket_free = 0;
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        char *key_ip;
        CC_Array *val_chs;
        CC_HASHTABLE_FOREACH(board, key_ip, val_chs, {
            if (strcmp(ip, key_ip) != 0) {
                continue;
            }
            if (socket_free == 0) {
                Channel *ch;
                cc_array_get_at(val_chs, 0, &ch);
                destroy_socket_stream(ch->socket);
                socket_free = 1;
            }
            cc_hashtable_remove(board, key_ip, NULL);
            free(key_ip);
            cc_array_destroy_cb(val_chs, channel_free);
            // printf("remove board %s\n", key_ip);
        });
    });
    int *k;
    CC_Array *val;
    CC_HASHTABLE_FOREACH(conf, k, val, {
        if (*(int*)k == key) {
            cc_hashtable_remove(conf, k, NULL);
            free(k);
            cc_array_destroy_cb(val, arr_ip_free);
            // printf("remove config %d\n", key);
        }
    });
    ReleaseMutex(hMutex);
    int rc = 0;

    time_t start_time = time(NULL);
    time_t timeout = 3;
    while (!InterlockedCompareExchange(&_lock, 1, 0)){
        Sleep(1000);
        time_t current_time = time(NULL);
        double elapsed_time = difftime(current_time, start_time);
        if (elapsed_time >= timeout) {
            return -2;
        }
    }
    if (get_board_conf_count() == 0) {
        rc = rst();
    }
    
    InterlockedCompareExchange(&_lock, 0, 1);
    if(rc != 0){
        return -2*10 + rc;
    }
    
    return 0;
}

/**
 * get channel count
 *
 * @param[in] key the key of a group of boards
 *
 * @return total channel_cout if the free was successful, <0 otherwise
 */
int get_channel_count(int key)
{
    WaitForSingleObject(hMutex,INFINITE);
    int count = 0;
    CC_Array *chs;
    cc_array_new(&chs);
    int rc = get_key_chs(key, chs);
    if (rc < 0) {
        // printf("Error get key %d channels: %d\n", key, rc);
        cc_array_destroy(chs);
        return -1;
    }
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->id == -1) {
            continue;
        }
        count++;
    });
    cc_array_destroy(chs);
    ReleaseMutex(hMutex);
    return count;
}


/**
 * config sample rate
 *
 * @param[in] key  the key of a group of boards
 * @param[in] sample_rate sample rate
 *
 * @return 0 if the free was successful, <0 otherwise
 */
int sample_rate(int key, int sample_rate)
{
    WaitForSingleObject(hMutex,INFINITE);
    int count = 0;
    CC_Array *chs;
    cc_array_new(&chs);
    int rc = get_key_chs(key, chs);
    if (rc < 0) {
        printf("Error get key %d channels:%d\n", key, rc);
        cc_array_destroy(chs);
        return -1;
    }
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        ch->sample_rate = sample_rate;
        if (ch->id != -1) {
            continue;
        }
        if (board_sample_rate(ch, sample_rate) < 0) {
            printf("Error key sample_rate:%d\n", key);
            cc_array_destroy(chs);
            return -2;
        }    
    });
    cc_array_destroy(chs);
    ReleaseMutex(hMutex);
    return 0;
}

/**
 * config trig mode
 *
 * @param[in] ip  the ip of the board
 * @param[in] chainMode 0 :starting, >trig_mode(0,1) 2:terminating, >trig_mode(1,0)
 *
 * @return 0 if the free was successful, <0 otherwise
 */
int trig_mode(const char *ip, int chainMode)
{
    WaitForSingleObject(hMutex,INFINITE);
    if (ip == NULL) {
        printf("Error ip is NULL\n");
        return -3;
    }
    if (chainMode < 0 || chainMode > 2) {
        printf("Error chainMode:%d\n", chainMode);
        return -4;
    }
    char *key_ip = strdup(ip);
    CC_Array *chs;
    if (cc_hashtable_get(board, key_ip, &chs) != CC_OK){
        printf("Error ip get channels: %s\n", ip);
        free(key_ip);
        return -1;
    }
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        ch->trig_mode = chainMode;
        // printf("trig_mode %d\n", ch->trig_mode);
    });
    Channel *ch_ctrl;
    cc_array_get_at(chs, 0, &ch_ctrl);
    int rc = board_trig_mode(ip, ch_ctrl);
    if (rc < 0) {
        printf("Error set trig_mode: %d\n", rc);
        free(key_ip);
        return -2;
    }
    free(key_ip);
    ReleaseMutex(hMutex);
    return 0;
}

/**
 * config sample enable
 *
 * @param[in] key  the key of a group of boards
 * @param[in] enable 0: disable, 1: enable
 *
 * @return 0 if the free was successful, <0 otherwise
 */
int sample_enable(int key, int enable)
{
    WaitForSingleObject(hMutex,INFINITE);
    CC_Array *chs;
    cc_array_new(&chs);
    int rc = get_key_chs(key, chs);
    if (rc < 0) {
        printf("Error get key %d channels\n:%d", key, rc);
        cc_array_destroy(chs);
        return -1;
    }
    Channel *tmp = NULL;
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->id != -1) {
            continue;
        }
        if (ch->trig_mode == TERMINATING || ch->trig_mode == INTERMEDIA) {
            rc = board_sample_enable(ch, enable);
            if (rc < 0){
                printf("Error key %d set sample_enable: %d\n", key, rc);
                cc_array_destroy(chs);
                return -2;
            }  
            printf("stream id:");
            show_stream_id(ch->stream_id, ch->stream_id_size);
            printf(" send sample enable:%d\n", enable);      
        } else if (ch->trig_mode == STARTING) {
            tmp = ch;
        } else {
            printf("Error key %d trig_mode %d\n", key, ch->trig_mode);
            cc_array_destroy(chs);
            return -3;
        }
    });
    if (tmp != NULL) {
        rc = board_sample_enable(tmp, enable);
        if (rc < 0){
            printf("Error key %d set sample_enable: %d\n", key, rc);
            cc_array_destroy(chs);
            return -4;
        }    
        // printf("stream id:");
        // show_stream_id(tmp->stream_id, tmp->stream_id_size);
        // printf(" send sample enable:%d\n", enable);       
    }
    cc_array_destroy(chs);
    ReleaseMutex(hMutex);
    return 0;
}

/**
 * read data
 *
 * @param[in] key  the key of a group of boards
 * @param[in] data the buffer to store the data
 * @param[in] data_size  the size of the buffer
 * @param[in] channel_size  the size of each channel
 *
 * @return 0 if the free was successful, -322 timeout, <0 otherwise
 */
int read(int key, void *data, int data_size, int channel_size)
{
    // WaitForSingleObject(hMutex,INFINITE);
    int rc = 0;
    size_t copy_size = sizeof(int)*channel_size;
    int channel_count = get_channel_count(key);
    if (channel_count*channel_size > data_size){
        // board_free(key);
        return -1;
    }
  
    CC_Array *arr_chs;
    cc_array_new(&arr_chs);
    rc = get_key_chs(key, arr_chs);
    if (rc < 0) {
        // printf("Error get key read channels:%d\n", rc);
        cc_array_destroy(arr_chs);
        // board_free(key);
        return -2;
    }
    channels_buffer_realloc(arr_chs);

    time_t start_time = time(NULL);
    time_t deadline = start_time + 5; 
    rc = channels_read(arr_chs, copy_size, deadline);
    if (rc < 0) {
        // printf("Error read channels:%d\n", rc);
        cc_array_destroy(arr_chs);
        // board_free(key);
        return -3*100+rc;
    }

    rc = data_copy(arr_chs, data, channel_size);
    if (rc < 0) {
        // printf("Error copy data:%d\n", rc);
        cc_array_destroy(arr_chs);
        // board_free(key);
        return -4;
    }
    cc_array_destroy(arr_chs);
    // ReleaseMutex(hMutex);
    return 0;
}

int data_copy(CC_Array *chs, void *data, int channel_size)
{
    int count = 0;
    void *channel;
    int convert_size = (channel_size / 8 + 1)*8;
    void *tmp = malloc(convert_size*sizeof(uint32_t));
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->id == -1) {
            continue;
        }

        endian_convert_simd((uint32_t *)ch->buffer,(uint32_t *)tmp, convert_size);
        // process_data_simd((uint32_t *)ch->buffer, channel_size, (ch->cal_enable==1? ch->gain_value:1), (float*)data + count*channel_size);
        process_data_simd((uint32_t *)tmp, convert_size, (ch->cal_enable==1? ch->gain_value: 1), (float*)tmp);
        memcpy((float*)data + count*channel_size,(float*)tmp , channel_size*sizeof(float));
        size_t copied_count = channel_size * sizeof(uint32_t);
        if (ch->read_count >= copied_count) {
            void *start = (void*)(ch->buffer + copied_count);
            int size = ch->read_count - copied_count;
            memcpy(ch->buffer, start, size);
            ch->read_count = size;
        }
        count++; 
    });
    free(tmp);
    return 0;
}

uint32_t convert_to_big_endian(uint8_t* data) 
{
    return (data[0] << 24 | data[1] << 16 | data[2] << 8| data[3]);
}

int channels_isconnected(CC_Array *chs)
{
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->isconnected == 0) {
            return -1;
        }
    })
    return 0;
}

int channels_read(CC_Array *chs, int copy_size, time_t deadline)
{ 
    if (channels_isconnected(chs) != 0) {
        printf("Error channels not all connected!\n");
        return -1;
    }
    Channel *ch;
    cc_array_get_at(chs, 0, &ch);
    void *socket = ch->socket;
    // printf("channels read socket: %p\n", socket);
    while(1) {
        time_t current_time = time(NULL);
        if (current_time >= deadline) {
            return -6;
        }
        if (channels_read_finish(chs, copy_size) == 0) {
            break;
        }
        
        char id[256] = {0};
        int rc = read_stream_id(socket, id, 256);
        if (rc <= 0){
            // printf("Error read stream id!: %d\n", rc);
            return -2*10 + rc;
        } 
        void *channel;
        int count = 0;
        CC_ARRAY_FOREACH(channel, chs, {
            Channel *ch = (Channel*)channel;
            if(cmp_stream_id(id, ch->stream_id, ch->stream_id_size) != 0){
                count++;
                if (count == cc_array_size(chs)) {
                    printf("Error read have no match stream id\n");
                    return -3;                   
                }
                continue;
            }

            int read_size = ch->buffer_size - ch->read_count;
            // int read_size = copy_size*3 - ch->read_count;
            int size = read_data(socket, ch->buffer + ch->read_count, read_size);
            if (size < 0){
                printf("Error read data\n");
                return -4;
            } 
            // printf("size:%d\n", size);
            ch->read_count += size;
            if (ch->buffer_size - ch->read_count < ch->buffer_size/3) {
                printf("Error too much data to read\n");
                return -5;
            }
            break;
        });   
        
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

int channels_read_finish(CC_Array *chs, int copy_size)
{
    int count = 0;
    int finish = 0;
    Channel *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->id == -1) {
            continue;
        }
        if (ch->ctrl == 0) {
            finish++;
        } else if (ch->read_count >= copy_size+8*4) {

            finish++;
        }
        count++;       
    }); 
    if (finish != count) {
        return -1;
    }
    return 0;
}

int channels_readcount_rst(CC_Array *chs)
{
    Channel *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        // if (ch->id == -1) {
        //     continue;
        // }
        ch->read_count = 0;
    });
    return 0;
}

int channels_buffer_realloc(CC_Array *chs)
{
    Channel *channel;
    // size_t size = 1024*1024*1;
    // int buffer_size = copy_size < size ? size : copy_size;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->id == -1) {
            continue;
        }
        int buffer_size = sizeof(int) * ch->sample_rate * 3;
        if (ch->buffer_size != buffer_size){
            ch->buffer = (uint8_t*)realloc(ch->buffer, buffer_size);
            memset(ch->buffer, 0, buffer_size);
            ch->buffer_size = buffer_size;      
        }
    });
    return 0;
}

int get_key_chs(int key, CC_Array *arr_chs)
{
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        // printf("Error key %d get arr_ip\n", key);
        return -1;
    } 
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *chs;
        if (cc_hashtable_get(board, ip, &chs) != CC_OK) {
            printf("Error ip %s get channels\n", ip);
            return -2;
        }
        void *channel;
        CC_ARRAY_FOREACH(channel, chs, {
            Channel *ch = (Channel*)channel;
            cc_array_add(arr_chs, ch);
        });
    });
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

/** 
 * Hash function for configuration string
 * 
 * @param[in] str Enter the string to be hashed
 * @return The hash value of the string
*/
int hash_string(const char* str) {
    int hash = 0;
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        hash = (hash << 5) + hash + str[i];
    }
    return (int)(hash & 0xFFFF);
}

int get_value(const char* param, const char* key) 
{
    const char* p = strstr(param, key);
    if (p != NULL) {
        return atoi(p + strlen(key));
    }
    return 0;
}

int board_add(char *ip, CC_Array *channels)
{
    char *ip_copy = strdup(ip);  
    cc_hashtable_add(board, (void*)ip_copy, (void*)channels);    
    return 0;  
}

int conf_add(const int key, CC_Array *arr_ip)
{
    int *k = (int*)malloc(sizeof(int));
    *k = key;
    cc_hashtable_add(conf, k, arr_ip);    
    return 0;
}

int parse_config(const char* config, CC_Array *arr_ip) 
{
    char* line;
    char* context1;
    int chainMode = TERMINATING;
    char ip[128] = {0};
    CC_Array *channels;

    char* config_copy = strdup(config);
    if (config_copy == NULL) {
        return -1; 
    }

    line = strtok_s(config_copy, "\n", &context1);
    while (line != NULL) {
        char* token;
        char* context2;
        // printf("line:%s\n", line);
        token = strtok_s(line, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -2; // Error parsing
        }
        if (strcmp(ip, token) != 0) {
            
            if (strlen(ip) != 0){
                board_add(ip, channels);
            }      
            chainMode = TERMINATING;      
            cc_array_new(&channels);
            memset(ip, 0, 128);
            memccpy(ip, token, 0, 128);   
            char *ip_copy = strdup(ip);  
            cc_array_add(arr_ip, (void*)ip_copy);
            // printf("ip:%s\n", ip);   
        }

        token = strtok_s(NULL, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -3; // Error parsing
        }

        int id = -1;
        if (sscanf(token, "ai%d", &id) != 1) {
            if (sscanf(token, "trig?chainMode=%d", &chainMode) == 1) {
                line = strtok_s(NULL, "\n", &context1);
                continue;
            } else {
                id = -2;
            }
        }

        int ctrl = -1, gain = -1, iepe = -1, coupling = -1, cal_enable = 1; 

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
                } else if (strncmp(token, "cal=", 4) == 0) {
                    cal_enable = get_value(token, "cal=");
                }
                token = strtok_s(NULL, "&", &context2);
            }
        }
        Channel *ch = (Channel*)malloc(sizeof(Channel));
        channel_init(ch);
        ch->id = id;
        if(ctrl == -1){
            free(config_copy);
            return -4;
        }
        ch->ctrl = ctrl;
        ch->gain = gain;
        ch->iepe = iepe;
        ch->coupling = coupling;
        ch->cal_enable = cal_enable;
        if (chainMode == 0) {
            ch->trig_mode = STARTING;
        } else if (chainMode == 1) {
            ch->trig_mode = INTERMEDIA;
        } else if (chainMode == 2) {
            ch->trig_mode = TERMINATING;
        } else {
            // printf("Error: Invalid chainMode: %d\n", chainMode);
            return -5;
        }
        
        cc_array_add(channels, (void*)ch);
        line = strtok_s(NULL, "\n", &context1);
    }
    board_add(ip, channels);

    free(config_copy);
    return 0;
}

int channel_init(Channel *channel)
{
    channel->id = 0;
    channel->socket = NULL;
    channel->sample_rate = 0;
    channel->stream_id_size = 256;
    channel->stream_id = (uint8_t*)calloc(sizeof(uint8_t), channel->stream_id_size);
    channel->buffer_size = 1024;
    channel->buffer = (uint8_t*)calloc(sizeof(uint8_t), channel->buffer_size);
    channel->read_count = 0;
    channel->ctrl = -1;
    channel->gain = -1;
    channel->iepe = -1;
    channel->coupling = -1;
    channel->trig_mode = TERMINATING;
    channel->cal_enable = 1;
    channel->isconnected = 0;
    channel->gain_value = 0.0;
    return 0;
}

int channel_free(Channel *channel)
{
    free(channel->stream_id);
    free(channel->buffer);
    free(channel);
    return 0;
}

int arr_ip_free(char *ip)
{
    free(ip);
    return 0;
}

// test is "ai6?"
int board_isall(int key)
{
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key get arr_ip:%d\n", key);
        return -1;
    }
    // int size = cc_array_size(arr_ip);
    // printf("size:%d\n", size);
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *channels;
        if (cc_hashtable_get(board, ip, &channels) != CC_OK){
            printf("Error key %s get channels\n", ip);
            return -2;
        }
        if (cc_array_size(channels) == 1){
            Channel *channel;
            cc_array_get_at(channels, 0, (void*)&channel);
            if (channel->id == -2) {
                for (int i = 0; i < 6; i++){
                    Channel *ch = (Channel*)malloc(sizeof(Channel));
                    channel_init(ch);
                    ch->id = i;
                    ch->ctrl = channel->ctrl;
                    ch->gain = channel->gain;
                    ch->iepe = channel->iepe;
                    ch->coupling = channel->coupling;
                    ch->trig_mode = channel->trig_mode;
                    ch->cal_enable = channel->cal_enable;
                    cc_array_add(channels, (void*)ch);
                }
                cc_array_remove_at(channels, 0, (void*)&channel);
                channel_free(channel);
            }
        }
    });
    return 0;
}

// add port:7601 control channel
int board_add_control(int key)
{
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key %d get arr_ip\n", key);
        return -1;
    }   
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *channels;
        if (cc_hashtable_get(board, ip, &channels) != CC_OK){
            printf("Error ip %s get board\n", ip);
            return -2;
        }
        Channel *ch;
        cc_array_get_at(channels, 0, (void*)&ch);
        int trig_mode = ch->trig_mode;
        Channel *ch_ctrl = (Channel*)malloc(sizeof(Channel));
        channel_init(ch_ctrl);
        ch_ctrl->id = -1;
        ch_ctrl->trig_mode = trig_mode;
        cc_array_add_at(channels, (void*)ch_ctrl, 0);
    });
    return 0;
}

void* create_socket_stream()
{
    void *socket = zmq_socket(context, ZMQ_STREAM);
    if (socket == NULL) {
        printf("Error create socket stream\n");
        return NULL;
    }
    int rc = 0;  
    rc = socket_timeout_set(socket, 1000);
    if (rc < 0) {
        // printf("status:%d\n", rc);
        zmq_close(socket);
        printf("Error set timeout\n");
        return NULL;
    }
    rc = socket_linger_set(socket, 0);
    if (rc < 0) {
        // printf("status:%d\n", rc);
        zmq_close(socket);
        printf("Error set linger\n");
        return NULL;
    }
    return socket;    
}

int destroy_socket_stream(void *socket_stream)
{
    zmq_close(socket_stream);
    return 0;
}

int board_add_socket_stream(int key)
{
    void *socket_stream = create_socket_stream();
    if (socket_stream == NULL){
        printf("Error create socket stream\n");
        return -1;
    }
    CC_Array *chs;
    cc_array_new(&chs);
    int rc = get_key_chs(key, chs);
    if (rc < 0) {
        printf("Error get key %d channels: %d\n", key, rc);
        cc_array_destroy(chs);
        destroy_socket_stream(socket_stream);
        return -2;
    }
    void *channel;
    CC_ARRAY_FOREACH(channel, chs, {
        Channel *ch = (Channel*)channel;
        if (ch->socket == NULL) {
            ch->socket = socket_stream;
        } else {   
            printf("Error socket stream already exists\n");
            cc_array_destroy(chs);
            destroy_socket_stream(socket_stream);
            return -3;
        }
    });
    cc_array_destroy(chs);
    return 0;    
}

int board_ch_ctrl_connect(key)
{
    int rc = 0;
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key get arr_ip:%d\n", key);
        return -1;
    }   
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *channels;
        if (cc_hashtable_get(board, ip, &channels) != CC_OK){
            printf("Error ip get board:%s\n", ip);
            return -2;
        }
        void *channel;
        CC_ARRAY_FOREACH(channel, channels, {
            Channel *ch = (Channel*)channel;
            if (ch->id != -1) {
                continue;
            }
            rc = channel_connect(ch, ip);
            if (rc < 0){
                printf("Error channel connect! id:%d rc:%d\n", ch->id, rc);
                return -3;
            }
        })
    });    
}

int board_connect(int key)
{
    int rc = 0;
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key %d get arr_ip\n", key);
        return -1;
    }   
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *channels;
        if (cc_hashtable_get(board, ip, &channels) != CC_OK){
            printf("Error ip get board:%s\n", ip);
            return -2;
        }
        void *channel;
        CC_ARRAY_FOREACH(channel, channels, {
            Channel *ch = (Channel*)channel;
            if (ch->id == -1) {
                continue;
            }
            rc = channel_connect(ch, ip);
            if (rc < 0){
                printf("Error channel %d connect!:%d\n", ch->id, rc);
                return -3;
            }
        })
    });  
    return 0;
}

int channel_connect(Channel *channel, const char *ip) 
{
    char addr[128] = {0};
    int rc = sprintf(addr, "tcp://%s:%d", ip, channel->id+7602);
    if (rc < 0) {
        // printf("Error addr syntax%d\n", channel->id);
        return -1;
    }
    // printf("addr: %s\n", addr);
    rc = zmq_connect(channel->socket, addr);
    if (rc != 0) {
        printf("Error zmq_connect Channel id:%d ip:%s\n", channel->id, ip);
        return -2;
    }
    rc = zmq_getsockopt(channel->socket, ZMQ_ROUTING_ID, channel->stream_id, &channel->stream_id_size);
    if (rc != 0) {
        printf("Error getting socket id option channel id:%d ip:%s\n", channel->id, ip);
        return -3;
    }
    rc = channel_read(channel);
    if (rc != 0){
        printf("Error channel_read Channel id:%d ip:%s rc:%d\n", channel->id, ip, rc);
        return -4;
    }
    channel->isconnected = 1;
    return 0;
}

int show_stream_id(uint8_t *data, size_t size)
{
    for (int i = 0; i < size; i++) {
        printf("%02x ", data[i]);
    }
    // printf("\n");
    return 0;
}

int channel_send(Channel *channel, const char *data) 
{
    zmq_send(channel->socket, channel->stream_id, channel->stream_id_size, ZMQ_SNDMORE);
    zmq_send(channel->socket, data, strlen(data), 0);
    Sleep(10);
    return 0;
}

int channel_read(Channel *channel)
{
    uint8_t id[256] = {0};
    int id_len = read_stream_id(channel->socket, id, 256);
    // printf("id_len:%d\n", id_len);
    if (id_len < 0) {
        printf("Error read_stream_id id:%d rc:%d\n", channel->id, id_len);
        return -1;
    }
    memset(channel->buffer, 0, channel->buffer_size);
    channel->read_count = 0;

    int data_size = read_data(channel->socket, channel->buffer, channel->buffer_size);
    // data_show(channel->buffer, data_size);
    if(data_size < 0){
        // printf("channel %d read data timeout!\n", channel->id);
        return -2;
    }   
    if(cmp_stream_id(channel->stream_id, id, id_len) != 0){   
        // printf("channel %d match\n", channel->channel_id);
        return -3;
    }     
    channel->read_count += data_size;
    return data_size;
}

int recv_more(void *socket)
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

int board_set_conf(int key)
{
    int rc = 0;
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key %d get arr_ip\n", key);
        return -1;
    }   
    char *ip;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *chs;
        if (cc_hashtable_get(board, ip, &chs) != CC_OK){
            printf("Error ip %s get board\n", ip);
            return -2;
        }   
        Channel *ch_ctrl;
        cc_array_get_at(chs, 0, &ch_ctrl);
        rc = board_set_conf_rst(ch_ctrl);
        if (rc < 0) {
            printf("Error ip %s board_set_conf_rst\n", ip);
            return -3;
        }
        Channel *channel;
        CC_ARRAY_FOREACH(channel, chs, {
            Channel *ch = (Channel*)channel;
            if (ch->id == -1) {
                continue;
            }  
            rc = board_set_conf_init(ch_ctrl, ch);
            if (rc < 0) {
                printf("Error ip %s board_set_conf_init\n", rc);
                return -4;
            }
        });
        rc = board_trig_mode(ip, ch_ctrl);
        if (rc < 0) {
            printf("Error board trig_mode:%s\n", ip);
            return -5;
        }
    });   
}

int board_set_conf_rst(Channel *ch_ctrl)
{ 
    char *msg = ">RST()\r\n";
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board reset:%d\n", rc);
        return -1;
    }  
    return 0;
}

int board_set_conf_init(Channel *ch_ctrl, Channel *ch)
{
    int rc = 0;
    rc = board_channel_ctrl(ch_ctrl, ch);
    if (rc < 0) {
        printf("Error channel %d channel ctrl\n", ch->id);
        return -2;
    }
    rc = board_gain(ch_ctrl, ch);
    if (rc < 0) {
        printf("Error channel %d gain\n", ch->id);
        return -3;
    }
    rc = board_gain_read(ch_ctrl, ch);
    if (rc < 0) {
        printf("Error channel %d gain read\n", ch->id);
        return -4;
    }
    rc = board_coupling(ch_ctrl, ch);
    if (rc < 0) {
        printf("Error channel %d coupling\n", ch->id);
        return -5;
    }
    rc = board_iepe(ch_ctrl, ch);
    if (rc < 0) {
        printf("Error channel %d iepe\n", ch->id);
        return -6;
    }
    return 0;
}

int board_channel_ctrl(Channel *ch_ctrl, Channel *ch)
{
    char msg[256] = {0};
    sprintf(msg, ">channel_ctrl(%d,%d)\r\n", ch->id, ch->ctrl);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){      
        printf("Error board channel ctrl: %d\n", rc);
        return -1;
    }
    return 0;
}

int board_gain(Channel *ch_ctrl, Channel *ch)
{
    if(ch->gain == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">gain_sel(%d,%d)\r\n", ch->id, ch->gain);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board gain\n");
        return -1;
    }
    return 0;
}

int board_gain_read(Channel *ch_ctrl, Channel *ch)
{
    char msg[256] = {0};
    sprintf(msg, ">gain_read(%d)\r\n", ch->id);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board gain read\n");
        return -1;
    }
    int id = 0;
    float gain_value = 0;
    // printf("board gain read: %s\n", ch_ctrl->buffer);
    int count = sscanf(ch_ctrl->buffer, "<gain_read(%d) gain=%f, OK!", &id, &gain_value);
    if (count != 2) {
        printf("Error board gain read count\n");
        return -2;
    }
    // printf("board gain read: %d %f\n", id, gain_value);
    ch->gain_value = gain_value;
    return 0;   
}

int board_coupling(Channel *ch_ctrl, Channel *ch)
{
    if (ch->coupling == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">coupling_sel(%d,%d)\r\n", ch->id, ch->coupling);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board coupling\n");
        return -1;
    }
    return 0;
}

int board_iepe(Channel *ch_ctrl, Channel *ch)
{
    if (ch->iepe == -1){
        return 0;
    }
    char msg[256] = {0};
    sprintf(msg, ">iepe_sel(%d,%d)\r\n", ch->id, ch->iepe);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board iepe\n");
        return -1;
    }
    return 0;
}

int board_sample_rate(Channel *ch_ctrl, int sample_rate)
{

    char msg[256] = {0};
    sprintf(msg, ">sample_rate(%d)\r\n", sample_rate);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error set sample_rate %d\n", sample_rate);
        return -1;        
    }
    return 0;
}

int board_trig_mode(char *ip, Channel *ch_ctrl)
{
    char *msg = NULL;
    if (ch_ctrl->trig_mode == STARTING) {
        msg = ">trig_mode(0,1)\r\n";
    } else if (ch_ctrl->trig_mode == INTERMEDIA) {
        msg = ">trig_mode(1,1)\r\n";
    } else if (ch_ctrl->trig_mode == TERMINATING) {
        msg = ">trig_mode(1,0)\r\n";
    }  else {
        printf("Error parse trig_mode\n");
        return -2;
    }
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board trig_mode\n");
        return -3;
    }
    // printf("stream id:");
    // show_stream_id(ch_ctrl->stream_id, ch_ctrl->stream_id_size);
    // printf(" ");
    // printf("trig_mode() ip: %s  msg: %s\n", ip, msg);
    return 0;
}

int board_sample_enable(Channel *ch_ctrl, int enable)
{
    if (ch_ctrl->isconnected == 0) {
        printf("Error board not connected\n");
        return -1;
    }
    char msg[256] = {0};
    sprintf(msg, ">sample_enable(%d)\r\n", enable);
    // channel_send(ch_ctrl, msg);
    int rc = board_cmd_set(ch_ctrl, msg);
    if (rc < 0){
        printf("Error board sample_enable\n");
        return -2;
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

int board_cmd_set(Channel *ch_ctrl, const char *data)
{
    if (ch_ctrl->isconnected == 0) {
        printf("Error board not connected\n");
        return -1;
    }
    // printf("Channel %d send: %s\n", ch_ctrl->id, data);
    channel_send(ch_ctrl, data);
    char *param_name = extract_parameter_name(data);

    int rc = channel_read(ch_ctrl);
    if (rc <= 0){
        printf("Error board receiving data:%d\n", rc);
        return -2;
    }
    // printf("Channel %d recv: %s\n", ch_ctrl->id, ch_ctrl->buffer);
    if (strstr(ch_ctrl->buffer, "OK") == NULL || strstr(ch_ctrl->buffer, param_name) == NULL){
        return -3;
    }     
    // memset(ch_ctrl->buffer, 0, ch_ctrl->buffer_size);
    // ch_ctrl->read_count = 0;
    return 0;
}

int show_board(const int key)
{
    if (cc_hashtable_size(board) == 0){
        printf("no board\n");
        return -1;
    }
    CC_Array *arr_ip;
    if (cc_hashtable_get(conf, &key, &arr_ip) != CC_OK){
        printf("Error key get arr_ip:%d\n", key);
        return -1;
    }
    int c = 150;
    for (int i = 0; i < c; i++) {
        printf("-");
        if (i == c/2) {
            printf("key: %d", key);
        }
    }
    printf("\n");
    // printf("--------------------------------------------------key: %d --------------------------------------------------\n", key);
    char *ip;
    int count = 0;
    CC_ARRAY_FOREACH(ip, arr_ip, {
        CC_Array *channels;
        if (cc_hashtable_get(board, ip, &channels) != CC_OK) {          
            printf("Error ip %s get channels\n", ip);
            return -2;
        }
        printf("---------- board: %d ----------\n", count);
        printf("key(ip): %s\n", ip);
        printf("value(channels):\n");
        count++;
        Channel *channel;
        CC_ARRAY_FOREACH(channel, channels, {
            Channel *ch = (Channel*)channel;
            printf("channel: %-3d  ", ch->id);
            // printf("socket addr:%p  ", ch->socket);
            // printf("isconnected: %-3d  ", ch->isconnected);
            printf("stream id:");
            show_stream_id(ch->stream_id, ch->stream_id_size);
            printf(" ");
            printf("buffer_size: %-8d  ", ch->buffer_size);
            printf("read_count: %-8d  ", ch->read_count);
            printf("ctrl: %-3d  ", ch->ctrl);
            printf("gain: %-3d  ", ch->gain);
            printf("iepe: %-3d  ", ch->iepe);
            printf("coupling: %-3d  ", ch->coupling);
            printf("cal_enable: %-3d  ", ch->cal_enable);
            if (ch->trig_mode == TERMINATING) {
                printf("trig_mode: TERMINATING  ");
            } else if (ch->trig_mode == INTERMEDIA) {
                printf("trig_mode: INTERMEDIA  ");
            } else if (ch->trig_mode == STARTING) {
                printf("trig_mode: STARTING  ");
            }
            printf("gain_value: %f", ch->gain_value);
            printf("\n");
        }); 
        // printf("\n");
    });
    // printf("-------------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < c; i++) {
        printf("-");
    }
    printf("\n");
    return 0;
}

int show_conf()
{
    if (cc_hashtable_size(conf) == 0){
        printf("no conf\n");
        return -1;
    }
    int *key;
    CC_Array *arr_ip;
    int count = 0;
    CC_HASHTABLE_FOREACH(conf, key, arr_ip, {
        printf("---------- conf: %d---------- \n", count);
        printf("key: %-8d\t", *(int*)key);
        printf("value(arr_ip): ");
        count++;
        char *ip;
        CC_ARRAY_FOREACH(ip, arr_ip, {
            printf("%-17s", (char*)ip);
            // printf("\n");
        });
        printf("\n");
    });
    return 0;
}
