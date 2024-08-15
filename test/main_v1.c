#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>

#include "zmq.h"
#include "stream.h"


int main() {

    FILE *fp = fopen("read_data.txt", "w");
    
    // 检查文件是否打开成功
    if (fp == NULL) {
        printf("无法打开文件!\n");
        return 1;
    }
    while(1){

    char *config = "192.168.0.6/ai1?ctrl=1\n";
    // char *config = "192.168.0.6/ai1?ctrl=1&gain=1&iepe=1&coupling=1\n";
    int rc = init(config, strlen(config)+1);
    // int rc = init(config, 1);

    if (rc < 0) {
        printf("init failed rc: %d\n", rc);
        return -1;
    }
    printf("init success!!!\n");

    int channel_count = get_channel_count();
    printf("channel_count:%d\n", channel_count);
    int channel_size = 10;
    int data_size = channel_count * channel_size;
    float *data = (float *)calloc(sizeof(float),data_size);
    int i = 1;

    sample_rate(100000);
    sample_enable(1);
    while(1){
        
        int rc = read(data, data_size, channel_size);
        if (rc < 0) {
            
            printf("read failed: %d\n", rc);
            return -1;
        }
        // uint8_t* data_u8 = (uint8_t*)data;
        for(int i=0;i<data_size;i++){
            fprintf(fp, "%f\n", data[i]);
            // fprintf(fp, "%02x", data_u8[i]);
            // if((i+1)%4==0){
            //     fprintf(fp, "\n");
            // }else{
                
            //     fprintf(fp, " ");
            // }
            // printf("%d\n", data[i]);
        }
        // printf("read done!!!!!\n");
        printf("read count: %d\n", i++);
        // Sleep(100);
        break;
    }
    
    rc = rst();
    if (rc < 0)
    {
        printf("rst failed\n");
    }
    
    printf("rst\n");  
    // Sleep(100);


    return 0;
    }      
    return 0;
}