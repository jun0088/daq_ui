#ifndef STREAM_H
#define STREAM_H
#include "zmq.h"

// Macro to detect endianness
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif


typedef enum {
    // INTERNAL = 0,
    // EXTERNAL = 1,
    STARTING = 0,       // (0,1)
    INTERMEDIA = 1,     // (1,1)
    TERMINATING = 2     // (1,0)
} TrigMode;



typedef struct Channel Channel;

typedef struct Channel {
    int id;
    void *socket;
    int sample_rate;
    uint8_t *stream_id;
    size_t stream_id_size;
    uint8_t *buffer;
    size_t buffer_size;
    size_t read_count;
    int ctrl;
    int gain;
    int iepe;
    int coupling;
    // 0:internal, 1:external
    int trig_mode;
    int cal_enable;
    // int primary;
    int isconnected;
    float gain_value;
};


#ifdef __cplusplus
extern "C"{
#endif
 
    int board_init(const char *config); // return is key
    int board_free(int key);
    int get_channel_count(int key);
    int sample_rate(int key, int sample_rate);
    int trig_mode(const char *ip, int chainMode);
    int sample_enable(int key, int enable);
    int read(int key, void *data, int data_size, int channel_size);
    int show_board(int key);
    int show_conf();
 

#ifdef __cplusplus
}
#endif

#endif 