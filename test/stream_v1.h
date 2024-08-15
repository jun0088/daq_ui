#ifndef STREAM_H
#define STREAM_H
#include "zmq.h"

typedef struct Channel Channel;

typedef struct Channel {
    char* ip;
    int channel_id;
    uint8_t *stream_id;
    size_t stream_id_size;
    uint8_t *buffer;
    size_t buffer_size;
    size_t read_count;
    int ctrl;
    int gain;
    int iepe;
    int coupling;
    int isconnected;
    float gain_value;
};

extern __declspec(dllexport) int init(const char *config, int size);
extern __declspec(dllexport) int read(void *data,  int data_size, int channel_size);
extern __declspec(dllexport) int rst();
extern __declspec(dllexport) int get_channel_count();
extern __declspec(dllexport) int sample_rate(int sample_rate);
extern __declspec(dllexport) int sample_enable(int enable);


int socket_timeout_set(void *socket, int timeout);
int socket_linger_set(void *socket, int linger);

int channel_init(Channel *channel);
int channel_free(Channel *channel);
int channels_init(Channel *channels, int size);
int channel_buffer_realloc(Channel *channel, int buf_size);
int channel_connect(void *socket, Channel *channel);
int channels_connect(Channel *channels, int size);
int channel_close(void *socket, Channel *channel);
int channel_send(void *socket, Channel *channel, const char *data);
int channel_read(void *socket, Channel *channel);
int channel_control_init(const char* ip);
int channel_control_free(Channel *channel_control);

int recv_more(void* socket);
int read_stream_id(void *socket, uint8_t *id, int id_size);
int read_data(void *socket, uint8_t *data, int data_size);
int cmp_stream_id(const void *a, const void *b, size_t size);
int data_show(uint8_t *data, size_t size);
int count_crlf(const char *str, int size);
int get_value(const char* param, const char* key);
int parse_config(char* config, Channel* channels);

// int board_close(Channel *channel_control);
int board_init();
// int board_start();
int board_rst();
int board_iepe(int ch, int iepe);
int board_gain(int ch, int gain);
int board_coupling(int ch, int coupling);
int board_channel_ctrl(int ch, int channel_ctrl);
// int board_sample_rate(int sample_rate);
// int board_sample_enable(int sample_enable);
char* extract_parameter_name(char* input_str);
int board_send_and_read(const char *data);

uint32_t convert_to_big_endian(uint8_t* data);
int channels_buffer_realloc(Channel *channels, size_t copy_size);
int channels_isconnected(Channel *channels);
int channels_readcount_rst(Channel *channels, int channel_count);
int channels_read_finish(Channel *channels, int channel_count);
int channel_buffer_write(Channel *channel, uint8_t *data, int data_size);
int channels_read(void *socket, Channel *channels, int channel_count);


#endif 