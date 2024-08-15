#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct Channel {
    char *ip;
    int channel_id;
    int enable;
    int gain;
    int iepe;
    int coupling;
} Channel;

// Helper function to parse key-value pairs
int get_value(const char* param, const char* key) {
    const char* p = strstr(param, key);
    if (p != NULL) {
        return atoi(p + strlen(key));
    }
    return 0; // Default value if key is not found
}

int parse(char* config, Channel* channels) {
    char* line;
    char* context1;
    int channel_index = 0;

    // Duplicate the string to avoid modifying the original config
    char* config_copy = strdup(config);
    if (config_copy == NULL) {
        return -1; // Memory allocation error
    }

    line = strtok_s(config_copy, "\n", &context1);
    while (line != NULL) {
        char* token;
        char* context2;

        // Extract device address and the rest of the parameters
        token = strtok_s(line, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -1; // Error parsing
        }
        printf("Device address1: %s\n", token);

        channels[channel_index].ip = strdup(token);
        // char* device_address = token; // e.g., "192.168.17.100"
        token = strtok_s(NULL, "/", &context2);
        if (token == NULL) {
            free(config_copy);
            return -1; // Error parsing
        }
        printf("Device address2: %s\n", token);
        // Extract channel number
        int channel_number;
        if (sscanf(token, "ai%d", &channel_number) != 1) {
            free(config_copy);
            return -1; // Error parsing
        }

        // Initialize default values
        int enable = -1, gain = -1, iepe = -1, coupling = -1;

        // Extract parameters
        token = strtok_s(token, "?", &context2);
        if (token != NULL) {
            token = strtok_s(NULL, "&", &context2);
            while (token != NULL) {
                if (strncmp(token, "enable=", 7) == 0) {
                    enable = get_value(token, "enable=");
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

        // Store the parsed information into the channels array
        channels[channel_index].channel_id = channel_number;
        channels[channel_index].enable = enable;
        channels[channel_index].gain = gain;
        channels[channel_index].iepe = iepe;
        channels[channel_index].coupling = coupling;
        channel_index++;

        // Get the next line
        line = strtok_s(NULL, "\n", &context1);
    }

    free(config_copy);
    return channel_index; // Return the number of parsed channels
}

int count_line(const char *str, int len) 
{  
    char* line;
    char* context1;
    int count = 0;
    // int len_read = 0;
    char* config_copy = strdup(str);
    if (config_copy == NULL) {
        return -1; // Memory allocation error
    }
    line = strtok_s(config_copy, "\n", &context1);  
    while (line != NULL) {
        printf("Line: %s\n", line);
        // len_read += strlen(line)+1;

        count++;
        // Get the next line
        line = strtok_s(NULL, "\n", &context1);          
    }
    // printf("len_read: %d\n", len_read);
    printf("len: %d\n", len);
    free(config_copy);
    return count;  
} 

uint32_t convert_to_big_endian(uint8_t* data) {
    uint32_t result = 0;

    // 检测系统字节序
    union {
        uint32_t value;
        uint8_t bytes[4];
    } test = { .value = 0x01020304 };

    // 如果系统是小端序, 则交换字节顺序
    if (test.bytes[0] == 0x04) {
        result = ((uint32_t)data[0] << 24) |
                 ((uint32_t)data[1] << 16) |
                 ((uint32_t)data[2] << 8) |
                 ((uint32_t)data[3]);
    }
    // 如果系统是大端序, 则直接赋值
    else {
        memcpy(&result, data, sizeof(uint32_t));
    }

    return result;
}

char* extract_parameter_name(char* input_str) {
    char* start = strchr(input_str, '>');
    if (start == NULL) {
        return NULL;
    }

    start++;  // 跳过'>'
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

int main() {
    // char *recv = ">sample_rate(200000)";
    // char *param_name = extract_parameter_name(recv);
    // printf("Parameter name: %s\n", param_name);
    // return 0;


    // uint8_t ip[4] = {0x4, 0x3, 0x2, 0x1};
    // uint32_t ip_int = convert_to_big_endian(ip);
    // printf("IP: %08X\n", ip_int);
    // return 0;

    char* config = "\
192.168.17.200/ai1?enable=1\n\
192.168.17.200/ai5?enable=1&gain=1&iepe=1&coupling=1\n\
192.168.17.200/ai6?enable=1&gain=1&iepe=1&coupling=1\n";

    // char *config = "192.168.17.200/ai6?enable=1\n";
    // char config[] = "192.168.17.100/ai0?enable=1&gain=10\n";
    int channel_count = count_line(config, strlen(config));
    printf("Channel count: %d\n", channel_count);

    Channel *channels = (Channel*)malloc(channel_count * sizeof(Channel)); // Assuming a maximum of 10 channels for this example

    int num_channels = parse(config, channels);

    // Printing the parsed channels for verification
    for (int i = 0; i < num_channels; i++) {
        printf("Channel %d:\n", i + 1);
        printf("  IP Address: %s\n", channels[i].ip);
        printf("  Channel ID: %d\n", channels[i].channel_id);
        printf("  Enable: %d\n", channels[i].enable);
        printf("  Gain: %d\n", channels[i].gain);
        printf("  IEPE: %d\n", channels[i].iepe);
        printf("  Coupling: %d\n", channels[i].coupling);
    }

    return 0;
}