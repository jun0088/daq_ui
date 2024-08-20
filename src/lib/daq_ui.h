#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <string>
#include <vector>
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

using namespace std;

class Channel {
    public:
        
        bool ctrl;
        bool gain;
        bool iepe;
        bool coupling;
        bool calenable;
        Channel(int id);
        int GetId();
        int ToInt(bool cfg);
    private:
        int _id;  
};

class Board {
    public:
        
        int trig = 0;
        Channel channel[7] = {-1, 0, 1, 2, 3, 4, 5};
        int channel_count = 7;
        Board();
        Board(int id, string ip);
        void ScanAll();
        // void SetIp(string ip);
        string GetIp();
        int GetId();
        string ConfigTxt();
    
    private:
        int _id;
        string _ip;
};

typedef enum {
    CTRL = 1,
    GAIN = 2,
    IEPE = 3,
    COUPLING = 4,
    CALENABLE = 5
} ChannelIndex;

void ShowBoardWindow(Board &board, int open_action);
string ScanBoard(); 
int BuildConfigTxt(vector<Board> &boardVec);

string extractIPAddress(const string& input);
string convertHexToIPAddress(const string& hex);
void Stringsplit(const string& str, const string& splits, vector<string>& res);
#endif

