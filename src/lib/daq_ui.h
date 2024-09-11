#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <string>
#include <vector>
#include "imgui.h"
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

using namespace std;
typedef struct ScrollingBuffer ScrollingBuffer;

// utility structure for realtime plot
typedef struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};

class UiChannel {
    public:
        ScrollingBuffer plot;
        bool show;
        bool ctrl;
        bool gain;
        bool iepe;
        bool coupling;
        bool calenable;
        UiChannel(int id);
        int GetId();
        int ToInt(bool cfg);
        int ToPlot(float *data, int len);
    private:
        int _id;  
};

class Board {
    public:
        
        int trig = -1;
        UiChannel channel[7] = {-1, 0, 1, 2, 3, 4, 5};
        int channel_count = 7;
        Board();
        Board(int id);
        void SyncAllConfig();
        // void SetIp(string ip);
        string GetIp();
        string GetMac();
        int GetId();
        string ConfigTxt();
        int ParseIpMac(string Dev);
    
    private:
        int _id;
        string _ip;
        string _mac;
};

typedef enum {
    CTRL = 1,
    GAIN = 2,
    IEPE = 3,
    COUPLING = 4,
    CALENABLE = 5
} ChannelIndex;

typedef enum {
    WAITING_CONFIG = 0,
    START_OK = 1,    
    START_ERROR = -1,
    READ_OK = 2,
    READ_ERROR = -2,
    STOP_OK = 3,
    STOP_ERROR = -3
} PlotFlag;

void ShowBoard(Board &board, int open_action);
string ScanBoard(); 
int BuildConfigTxt(vector<Board> &boardVec);
void ShowBoardPlotsWindows(vector<Board> &boardVec, int SampleCount);
string GetConfigTxt(vector<Board> &boardVec);
int Data2Plots(vector<Board> &boardVec, float *data, int data_size, int SampleCount);
string extractIp(const string& input);
string extractMac(const string& input) ;
string convertHexToIPAddress(const string& hex);
void Stringsplit(const string& str, const string& splits, vector<string>& res);
int Start(const char *config, int SampleRate, int SampleEnable);
int boardVecInit(vector<Board> &boardVec);





#endif

