#include <stdio.h>
#include <math.h> 
#include "daq_ui.h"
#include "daq.h"
#include "imgui.h"
#include "imconfig.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"
#include "implot_internal.h"
#include <d3d11.h>
#include <tchar.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace std;
// Main code
int main(int, char**)
{

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();


    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    static vector<Board> boardVec;
    boardVecInit(boardVec);



    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        
        ImGui::ShowDemoWindow(&show_demo_window);
        ImPlot::ShowDemoWindow();
        

        // static bool trig = false;
        {
            ImGui::Begin("Config"); 
            int open_action = -1;

            if (ImGui::Button("Expand"))
                open_action = 1;
            ImGui::SameLine();
            if (ImGui::Button("Collapse"))
                open_action = 0;
            ImGui::Separator();

            static int SampleCount = 100;
            ImGui::InputInt("Sample Count", &SampleCount);
            
            static int SampleRate = 5000;
            ImGui::InputInt("Sample rate", &SampleRate);

            static int SampleEnable = 1;
            ImGui::InputInt("Sample enable", &SampleEnable);


            // static bool plot_open = false;
            static int key = 0;
            static int channel_count = 0;
            static float *data = NULL;
            static int data_size = 0;
            static int plot_flag = WAITING_CONFIG;
            if (ImGui::Button("Start")) {
                string config = GetConfigTxt(boardVec);
                key = Start(config.c_str(), SampleRate, SampleEnable);
                if (key < 0) {
                    plot_flag = START_ERROR;
                } else {           
                    channel_count = get_channel_count(key);
                    data_size = SampleCount * channel_count;
                    data = new float[data_size];
                    plot_flag = START_OK;
                }
            }
            if (key && (plot_flag == START_OK || plot_flag == READ_OK)) {
                int rc = read(key, data, data_size, SampleCount);
                if (rc < 0) {
                    printf("read failed: %d\n", rc);
                    plot_flag = READ_ERROR;
                } else {
                    rc = Data2Plots(boardVec, data, data_size, SampleCount);
                    if (rc < 0) {
                        printf("Data2Plots failed: %d\n", rc);
                        plot_flag = READ_ERROR;
                    } else {
                        plot_flag = READ_OK;
                    }
                }    
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                int rc = board_free(key);
                if (rc < 0) {
                    plot_flag = STOP_ERROR;
                } else {
                    delete[] data;
                    plot_flag = STOP_OK;
                }  
            }
            


            ShowBoardPlotsWindows(boardVec, SampleCount);

                

            static int config_flag = 1;
            if (ImGui::Button("BuildConfigTxt")) {
                config_flag = BuildConfigTxt(boardVec);
            }
            if (config_flag == 0) {
                ImGui::SameLine();
                ImGui::Text("BuildConfigTxt done!");
            } else if (config_flag == -1) {
                ImGui::SameLine();
                ImGui::Text("BuildConfigTxt failed!");
            }
            ImGui::Separator();         


            for (int i = 0; i < boardVec.size(); i++) {
                ShowBoard(boardVec[i], open_action);
            }

            ImGui::End();      
        }

        // {
            
        //     ImGui::Begin("Board Plots"); 

        //     // Checkbox for each channel
        //     int num = 0;
        //     for (int i = 0; i < boardVec.size(); i++) {
        //         ImGui::Text("Board %d (%s):", i, boardVec[i].GetIp().c_str());
        //         for (int j = 1; j < boardVec[i].channel_count; j++) {            
        //             if (boardVec[i].channel[j].ctrl) {
        //                 ImGui::SameLine();
        //                 char label[256] = {0};
        //                 sprintf(label, "%d: Ch %d", num++, j);
        //                 ImGui::Checkbox(label, &boardVec[i].channel[j].show);    
        //             }
        //         }
        //         ImGui::Separator();
        //     }

        //     // static bool paused = false;
        //     // static ScrollingBuffer dataAnalog[2];
        //     // static bool showAnalog[2] = {true, false};


        //     // ImGui::Checkbox("analog_0",  &showAnalog[0]);  ImGui::SameLine();
        //     // ImGui::Checkbox("analog_1",  &showAnalog[1]);

        //     // static float t = 0;
        //     // if (!paused) {
        //     //     t += ImGui::GetIO().DeltaTime;

        //     //     if (showAnalog[0])
        //     //         dataAnalog[0].AddPoint(t, sinf(2*t));
        //     //     if (showAnalog[1])
        //     //         dataAnalog[1].AddPoint(t, cosf(2*t));
        //     // }
        //     // char label[32];
        //     // if (ImPlot::BeginPlot("##Digital")) {
        //     //     ImPlot::SetupAxisLimits(ImAxis_X1, t - 10.0, t, paused ? ImGuiCond_Once : ImGuiCond_Always);
        //     //     ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1);

        //     //     for (int i = 0; i < 2; ++i) {
        //     //         if (showAnalog[i]) {
        //     //             snprintf(label, sizeof(label), "analog_%d", i);
        //     //             if (dataAnalog[i].Data.size() > 0)
        //     //                 ImPlot::PlotLine(label, &dataAnalog[i].Data[0].x, &dataAnalog[i].Data[0].y, dataAnalog[i].Data.size(), 0, dataAnalog[i].Offset, 2 * sizeof(float));
        //     //         }
        //     //     }
        //     // }
                
        //     // ImPlot::EndPlot();
        //     ImGui::End();
        // }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
