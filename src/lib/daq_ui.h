#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <string>
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

class Channel {
    public:
        
        bool ctrl;
        bool gain;
        bool iepe;
        bool coupling;
        bool calenable;
        Channel(int id);
        int get_id();

    private:
        int _id;  
};

class Board {
    public:
        std::string ip;
        bool trig;
        Channel channel[9] = {0,1,2,3,4,5,6,7,8};
        Board();
        void scan_all();
};

typedef enum {
    CTRL = 1,
    GAIN = 2,
    IEPE = 3,
    COUPLING = 4,
    CALENABLE = 5
} ChannelIndex;

void ShowBoardWindow(Board &board);

#endif

