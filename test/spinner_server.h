#ifndef CLIENT_H
#define CLIENT_H

// #define LOG_NAME "client.log"


// int server_fd_init(int port, char *ip);
// int client_fd_init(int server_socket);

// int send_header(int connect_fd, size_t data_len);

// void show_msg(uint8_t *buf, size_t len);
// #include <stdint.h>
typedef struct
{
    void *zmq_sock;
    uint8_t *id;
    size_t id_size;
    uint8_t *buf;
    int used;
    size_t buf_size;
} report_t;

report_t *create_report(void *context, const char *server);
void destroy_report(report_t *report);
size_t get_id(report_t *report);
void write_report(report_t *report, const uint8_t *message, size_t size);
void flush_report(report_t *report);
void close_report(report_t *report);

int write_thermoelectric_data(report_t *report, uint8_t *data, size_t len);
int write_thermoelectric_alarm_value(report_t *report, uint8_t *data, size_t len);
int write_rotate_speed(report_t *report, uint8_t *data, size_t len);
int write_fault_point(report_t *report, uint8_t *data, size_t len);
int write_instant_speed(report_t *report, uint8_t *data, size_t len);

#endif