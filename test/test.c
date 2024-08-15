#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "zmq.h"

typedef enum Status {
    IDLE =0,
    RUNNING,
    STOPPED
};

typedef struct Channel Channel;

typedef struct Channel
{
    int id[256];
    char * ip;
    int channel_id;
    float * data;
    int data_size;
};

typedef struct Control Control;

typedef struct Control{
    int id[256];
    char * ip;
};

typedef struct Task Task;

typedef struct Task {
    int task_id;
    Channel * channel;
    int channel_count;
    Control * control;
    int control_count;
    bool ready;
};

Task * current_task = NULL;

int create_task(char * description){
    // description: 192.168.1.100/ai0:7
    

}

int config_sampling(int channel_count,int sample_rate, int sample_per_time){

}

// int start(){

// }

// int stop(){

// }

int read_data(int task_id, float *data){

}

int destroy_task(int task_id){

}

int main(){
    
}