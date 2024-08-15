#include "daq_ui.h"
#include "imgui.h"
#include <stdio.h>
#include <string>

Channel::Channel(int id)
{
    _id = id;
    ctrl = false;
    gain = false;
    iepe = false;
    coupling = false;
    calenable = false;   
}

int Channel::get_id()
{
    return _id;
}

Board::Board()
{
    trig = false;
    ip = "192.168.0.1";
}

void Board::scan_all()
{
    for (int i = 1; i < 9; i++) {
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
    // for (int i = 1; i < 9; i++) {
    //     if (!channel[i].ctrl) {
    //         channel[0].ctrl = false;
    //     }
    //     if (!channel[i].gain) {
    //         channel[0].gain = false;
    //     }
    //     if (!channel[i].iepe) {
    //         channel[0].iepe = false;
    //     }
    //     if (!channel[i].coupling) {
    //         channel[0].coupling = false;
    //     }
    //     if (!channel[i].calenable) {
    //         channel[0].calenable = false;
    //     }
    // }
}

void ShowBoardWindow(Board &board)
{
    char title[64] = {0};
    sprintf(title, "Board (%s)", board.ip.c_str());

    if (ImGui::TreeNode(title)) {
        ImGui::Checkbox("trig", &board.trig);
        static ImGuiTableFlags flags1 = ImGuiTableFlags_BordersV;
        if (ImGui::BeginTable("board", 6, flags1)){
            ImGui::TableSetupColumn("Channel");
            ImGui::TableSetupColumn("ctrl(1)");
            ImGui::TableSetupColumn("gain(2)");
            ImGui::TableSetupColumn("iepe(3)");
            ImGui::TableSetupColumn("coupling(4)");
            ImGui::TableSetupColumn("cal(5)");
            ImGui::TableHeadersRow();
            for (int row = 0; row < 9; row++) {
                ImGui::TableNextRow();
                for (int column = 0; column < 6; column++) {
                    ImGui::TableSetColumnIndex(column);
                    if (column == 0) {
                        if (row == 0) {      
                            ImGui::Text("All");
                        } else {
                            ImGui::Text("Ch%d", row);                                
                        }
                    } else {
                        static bool *tmp = NULL;
                        board.scan_all();
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
        
    }
}
