#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include "spinner_server.h"
#include "zmq.h"



report_t *create_report(void *context, const char *server)
{
    // context = zmq_ctx_new();
    void *s = zmq_socket(context, ZMQ_STREAM);
    //
    // int rc = zmq_connect(s, server);
    int rc = zmq_bind(s, server);

    report_t *report = (report_t *)malloc(sizeof(report_t));
    report->zmq_sock = s;    
    report->id = (uint8_t *)malloc(256);
    report->id_size = 0;
    report->buf_size = 16*1024;    
    report->buf = malloc(report->buf_size);
    report->used = 0;

    return report;
}

void destroy_report(report_t *report)
{
    int closed = zmq_close(report->zmq_sock);
    if (closed != 0)
    {
        int err = zmq_errno();
        printf("Failed to close socket: %s\n", zmq_strerror(err));
    }
    free(report->id);
    free(report->buf);
    free(report);
}

size_t get_id(report_t *report)
{
    uint8_t id[256];
    zmq_msg_t id_msg;
    zmq_msg_init(&id_msg);
    zmq_msg_recv(&id_msg, report->zmq_sock, 0);
    void *id_data = zmq_msg_data(&id_msg);
    size_t id_size = zmq_msg_size(&id_msg);
    zmq_msg_close(&id_msg); 

    memcpy(id, id_data, id_size);
    if(id_size == 0) {
        printf("id:%x disconncted!\n", id);
        return -1;
    }     
    printf("id:%x has connected!\n", id);       
    memcpy(report->id, id, id_size);
    report->id_size = id_size;

    return 0;
}

void write_report(report_t *report, const uint8_t *message, size_t size)
{

    if (report->used + size > report->buf_size)
    {
        size_t new_size = ((report->used + size) / 4096 + 1) * 4096;
        printf("used:%d realloc %d\r\n", report->used, new_size);
        report->buf = realloc(report->buf, new_size);
        report->buf_size = new_size;
    }

    memcpy(report->buf + report->used, message, size);
    report->used += size;
    // printf("used:%d\r\n", report->used);
}

void flush_report(report_t *report)
{
    int size = htons(report->used);

    zmq_send(report->zmq_sock, report->id, report->id_size, ZMQ_SNDMORE);
    zmq_send(report->zmq_sock, &size, sizeof(uint16_t), 0);

    zmq_send(report->zmq_sock, report->id, report->id_size, ZMQ_SNDMORE);
    zmq_send(report->zmq_sock, report->buf, report->used, 0);

    printf("flush\r\n");
    report->used = 0;
}

void close_report(report_t *report)
{
    zmq_send(report->zmq_sock, report->id, report->id_size, ZMQ_SNDMORE);
    zmq_send(report->zmq_sock, NULL, 0, 0);
    printf("close\r\n");
}


int write_thermoelectric_data(report_t *report, uint8_t *data, size_t len)
{
    const uint8_t thermoelectric_data[8] = {0xc8, 0xc8, 0xb5, 0xe7, 0xca, 0xfd, 0xbe, 0xdd};    
    write_report(report, data, len);
    write_report(report, thermoelectric_data, 8);
    return 0;
}

int write_thermoelectric_alarm_value(report_t *report, uint8_t *data, size_t len)
{
    const uint8_t thermoelectric_alarm_value[10] = {0xc8, 0xc8, 0xb5, 0xe7, 0xb1, 0xa8, 0xbe, 0xaf, 0xd6, 0xb5};
    write_report(report, data, len);
    write_report(report, thermoelectric_alarm_value, 10);
    return 0;
}

int write_rotate_speed(report_t *report, uint8_t *data, size_t len)
{
    const uint8_t rotate_speed[4] = {0xd7, 0xaa, 0xcb, 0xd9};    
    write_report(report, data, len);
    write_report(report, rotate_speed, 4);
    return 0;
}

int write_fault_point(report_t *report, uint8_t *data, size_t len)
{
    const uint8_t fault_point[6] = {0xb9, 0xca, 0xd5, 0xcf, 0xb5, 0xe3};
    write_report(report, data, len);
    write_report(report, fault_point, 6);
    return 0;
}

int write_instant_speed(report_t *report, uint8_t *data, size_t len)
{
    const uint8_t instant_speed[8] = {0xcb, 0xb2, 0xca, 0xb1, 0xd7, 0xaa, 0xcb, 0xd9};
    write_report(report, instant_speed, 8);
    write_report(report, data, len);
    return 0;
}