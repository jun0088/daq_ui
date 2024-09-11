#include "daq_ui.h"
#include "imgui.h"
#include "daq.h"
#include "implot.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>

#include <winsock2.h>
#include <Windows.h>
 
#pragma comment(lib,"ws2_32.lib")

using namespace std;

UiChannel::UiChannel(int id)
{
    _id = id;
    show = true;
    ctrl = false;
    gain = false;
    iepe = false;
    coupling = false;
    calenable = false;   
}

int UiChannel::GetId()
{
    return _id;
}

int UiChannel::ToInt(bool cfg)
{
    if (cfg) {
        return 1;
    } else {
        return 0;
    }
}

int UiChannel::ToPlot(float *data, int len)
{
    for (int i = 0; i < len; i++) {
        plot.AddPoint(i/1.0, data[i]); 
        printf("%f\n", data[i]);
    }
    return 0;
}

Board::Board()
{
    _id = 0;
    _ip = "127.0.0.1";
}

Board::Board(int id, string ip)
{

    _id = id;
    _ip = ip;
}

// void Board::SetIp(string ip)
// {
//     _ip = ip;
// }

string Board::GetIp()
{
    return _ip;
}

int Board::GetId()
{
    return _id;
}

void Board::SyncAllConfig()
{
    for (int i = 1; i < channel_count; i++) {
        if (channel[0].ctrl) {
            channel[i].ctrl = true;
        }
        if (channel[0].gain) {
            channel[i].gain = true;
        }
        if (channel[0].iepe) {
            channel[i].iepe = true;
        }
        if (channel[0].coupling) {
            channel[i].coupling = true;
        }
        if (channel[0].calenable) {
            channel[i].calenable = true;
        }
    }
}

string Board::ConfigTxt()
{
    string txt = "";
    if (this->trig == -1) {
        return txt;
    }
    // 192.168.0.173/trig?chainMode=0
    txt += this->GetIp() + "/trig?chainMode=" + to_string(this->trig) + "\n";
    for (int i = 1; i < channel_count; i++) {
        // 192.168.0.174/ai0?ctrl=1&gain=0&iepe=1&coupling=1&cal=0
        if (!channel[i].ctrl) {
            continue;
        }
        
        txt += this->GetIp() + "/ai" + to_string(this->channel[i].GetId()) + "?ctrl=1";
        txt += "&gain=" + to_string(channel[i].ToInt(channel[i].gain));
        txt += "&iepe=" + to_string(channel[i].ToInt(channel[i].iepe));
        txt += "&coupling=" + to_string(channel[i].ToInt(channel[i].coupling));
        txt += "&cal=" + to_string(channel[i].ToInt(channel[i].calenable));
        txt += "\n";
    }
    return txt;
}


void ShowBoard(Board &board, int open_action)
{
    if (open_action != -1)
        ImGui::SetNextItemOpen(open_action != 0);
    char title[64] = {0};
    sprintf(title, "Board %d (%s)", board.GetId(), board.GetIp().c_str());

    if (ImGui::TreeNode(title)) {

        ImGui::Text("trig: "); ImGui::SameLine();
        ImGui::RadioButton("STARTING", &board.trig, 0);     ImGui::SameLine();
        ImGui::RadioButton("INTERMEDIA", &board.trig, 1);   ImGui::SameLine();
        ImGui::RadioButton("TERMINATING", &board.trig, 2);  ImGui::SameLine();
        ImGui::RadioButton("UNABLE", &board.trig, -1);    

        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV;
        static int col = 6;
        if (ImGui::BeginTable("board", col, flags1)){
            ImGui::TableSetupColumn("Channel");
            ImGui::TableSetupColumn("ctrl(1)");
            ImGui::TableSetupColumn("gain(2)");
            ImGui::TableSetupColumn("iepe(3)");
            ImGui::TableSetupColumn("coupling(4)");
            ImGui::TableSetupColumn("cal(5)");
            ImGui::TableHeadersRow();
            for (int row = 0; row < board.channel_count; row++) {
                ImGui::TableNextRow();
                for (int column = 0; column < col; column++) {
                    ImGui::TableSetColumnIndex(column);
                    if (column == 0) {
                        if (row == 0) {      
                            ImGui::Text("All");
                        } else {
                            ImGui::Text("Ch%d", row-1);                                
                        }
                    } else {
                        static bool *tmp = NULL;
                        board.SyncAllConfig();
                        if (column == CTRL) {
                            tmp = &(board.channel[row].ctrl);
                        } else if (column == GAIN) {
                            tmp = &(board.channel[row].gain);
                        } else if (column == IEPE) {
                            tmp = &(board.channel[row].iepe);
                        } else if (column == COUPLING) {
                            tmp = &(board.channel[row].coupling);
                        } else if (column == CALENABLE) {
                            tmp = &(board.channel[row].calenable);
                        }
                        char str[10] = {0};
                        sprintf(str, "%d,%d", row, column);
                        ImGui::Checkbox(str, tmp);
                    }
                }                    
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
        ImGui::Separator();
    }
}


string ScanBoard() 
{

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("WSAStartup failed\n");
		return "";
	}
 
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == SOCKET_ERROR)
	{
		printf("create socket failed\n");
		return "";
	}
    int timeout = 1000; 
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        printf("setsockopt failed\n");
        return "";
    }
 
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(1112);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
 
	int rc = bind(sock, (sockaddr*)&serverAddr, sizeof(sockaddr));
	if (rc == SOCKET_ERROR){
		printf("bind failed\n");
		return "";
    }

	char *msg = ">get_dev\r\n";
	sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(1111);
	clientAddr.sin_addr.S_un.S_addr = inet_addr("192.168.0.255");
	rc = sendto(sock, msg, strlen(msg), 0, (sockaddr*)&clientAddr, sizeof(sockaddr));
	if (rc == SOCKET_ERROR) {
		printf("sendto failed\n");
		return "";
	}

	char buf[1024];
    // memset(buf, 0, 1024);
    // int read_index = 0;
    string arrIp;
    while (1) {
        memset(buf, 0, 1024);
        int size = recvfrom(sock, buf, 1024, 0, NULL, NULL);
        if (size < 0) {
            // printf("read done\n");
            break;
        } 
        if (arrIp.size() > 0) {
            arrIp += ",";
        }
        string tmp_ip = extractIPAddress(buf);
        string ip = convertHexToIPAddress(tmp_ip);
        arrIp += ip;
        // printf("recv size: %d \nrecvfrom:%s\n", size, buf);       
    }
    closesocket(sock);
    WSACleanup();

    // std::cout << "arrIp: " << arrIp << std::endl;
    return arrIp;
}

std::string extractIPAddress(const std::string& input) 
{
    std::regex pattern(R"(ip=(\w+))");
    std::smatch match;
    if (std::regex_search(input, match, pattern)) {
        return match[1];
    }
    return "";
}

std::string convertHexToIPAddress(const std::string& hex) 
{
    std::vector<int> octets;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string octet = hex.substr(i, 2);
        int value = std::stoi(octet, nullptr, 16);
        octets.push_back(value);
    }

    std::string ipAddress;
    for (size_t i = 0; i < octets.size(); i++) {
        ipAddress += std::to_string(octets[i]);
        if (i < octets.size() - 1) {
            ipAddress += ".";
        }
    }
    return ipAddress;
}


void Stringsplit(const string& str, const string& splits, vector<string>& res)
{
	if (str == "")		return;
	string strs = str + splits;
	size_t pos = strs.find(splits);
	int step = splits.size();
 
	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}

string GetConfigTxt(vector<Board> &boardVec)
{
    string txt = "";
    for (int i = 0; i < boardVec.size(); i++) {
        txt += boardVec[i].ConfigTxt();
    }
    cout << txt << endl;
    return txt;
}

int BuildConfigTxt(vector<Board> &boardVec)
{
    string txt = GetConfigTxt(boardVec);
    ofstream file;
    file.open("config.txt");
    if (!file.is_open()) {
        std::cerr << "open file" << std::endl;
        return -1;
    }   
    file << txt;
    file.close();
    cout << "build config.txt done" << endl;
    return 0;
}

void ShowBoardPlotsWindows(vector<Board> &boardVec, int SampleCount)
{
    // if (!ImGui::Begin("Board Plots", plot_open)) {
    //     ImGui::End();
    //     return; 
    // }
    ImGui::Begin("Board Plots");
    // Checkbox for each channel
    int num = 0;
    for (int i = 0; i < boardVec.size(); i++) {
        ImGui::Text("Board %d (%s):", i, boardVec[i].GetIp().c_str());
        for (int j = 1; j < boardVec[i].channel_count; j++) {            
            if (boardVec[i].channel[j].ctrl) {
                ImGui::SameLine();
                char label[256] = {0};
                sprintf(label, "%d: Ch %d", num++, j);
                ImGui::Checkbox(label, &boardVec[i].channel[j].show);    
            }
        }
        ImGui::Separator();
    }

    char label[32];
    if (ImPlot::BeginPlot("##Digital")) {
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, SampleCount, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -3, 3);
        int num = 0;
        for (int i = 0; i < boardVec.size(); i++) {
            for (int j = 1; j < boardVec[i].channel_count; j++) {
                if (boardVec[i].channel[j].ctrl && boardVec[i].channel[j].show) {
                    sprintf(label, "%d Ch %d", num++, j);
                    ScrollingBuffer &plot = boardVec[i].channel[j].plot;
                    if (boardVec[i].channel[j].plot.Data.size() == SampleCount) {
                        // ImPlot::PlotLine(label, boardVec[i].channel[j].plot.Data[0].x, boardVec[i].channel[j].plot.Data[0].y, SampleCount, 0, boardVec[i].channel[j].plot.Offset, 2 * sizeof(float));
                        ImPlot::PlotLine(label, &plot.Data[0].x, &plot.Data[0].y, SampleCount, 0, plot.Offset, 2 * sizeof(float));
                    }
                }
            }
        }
    }
        
    ImPlot::EndPlot();
    ImGui::End();   
}

int GetUIChannelCount(vector<Board> &boardVec)
{
    int num = 0;
    for (int i = 0; i < boardVec.size(); i++) {
        for (int j = 1; j < boardVec[i].channel_count; j++) {
            if (boardVec[i].channel[j].ctrl) {
                num++;
            }
        }
    }
    return num;
}

int Data2Plots(vector<Board> &boardVec, float *data, int data_size, int SampleCount)
{
    int count = GetUIChannelCount(boardVec);
    if (count * SampleCount != data_size) {
        return -1;
    }
    int num = 0;
    for (int i = 0; i < boardVec.size(); i++) {
        for (int j = 1; j < boardVec[i].channel_count; j++) {
            if (boardVec[i].channel[j].ctrl) {
                boardVec[i].channel[j].ToPlot(data + num*SampleCount, SampleCount);
                num++;
            }
        }
    }
    return 0;
}

int Start(const char *config, int SampleRate, int SampleEnable)
{
    int key = board_init(config);
    if (key < 0) {
        return -1;
    }
    int channel_count = get_channel_count(key);
    if (channel_count < 0) {
        board_free(key);
        return -2;
    }
    int rc = sample_rate(key, SampleRate);
    if (rc < 0) {
        board_free(key);
        return -3;
    }
    rc = sample_enable(key, SampleEnable);
    if (rc < 0) {
        board_free(key);
        return -4;
    }   
    return key;
}
