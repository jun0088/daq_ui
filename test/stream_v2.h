#ifndef STREAM_H
#define STREAM_H
#include "zmq.h"

typedef struct Channel Channel;

typedef struct Channel {
    int id;
    uint8_t *stream_id;
    size_t stream_id_size;
    uint8_t *buffer;
    size_t buffer_size;
    size_t read_count;
    int ctrl;
    int gain;
    int iepe;
    int coupling;
    int trig_mode;
    // int primary;
    int isconnected;
    float gain_value;
};

extern __declspec(dllexport) int init(const char *config, int len); // return is key
extern __declspec(dllexport) int read(int key, void *data,  int data_size, int channel_size);
extern __declspec(dllexport) int rst(int key);
extern __declspec(dllexport) int get_channel_count(int key);
extern __declspec(dllexport) int sample_rate(int key, int sample_rate);
extern __declspec(dllexport) int sample_enable(int key, int enable);
extern __declspec(dllexport) int trig_mode(const char *ip, int len, int externl, int internal);


#endif 