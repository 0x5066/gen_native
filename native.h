#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0900

#include <windows.h>
#include <commctrl.h>
#include "wacup/gen.h"
#include "wacup/wa_ipc.h"
#include "resource.h"
#include <vector>
#include <iterator>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#define WIN32_LEAN_AND_MEAN

/* P L U G I N   D E F I N E S */
#define PLUGIN_TITLE L"Native Main Window Plugin"

#define PLUGIN_CAPTION "Main Window (Native)"
// Timer ID
#define ID_TIMER 1

HWND hwnd_winamp = FindWindow("Winamp v1.x", NULL); // find winamp

// Function declarations
void config(void);
void quit(void);
int init(void);
void OpenMyDialog();
void DrawMainBox(HWND hMainBox, int res);
void DrawBitrate(HWND hBitrate, int res, int bitr);
void DrawSamplerate(HWND hSamplerate, int res, int smpr);
void DrawSongTicker(HWND hSongTicker);
std::wstring formatTime(int milliseconds);
//const wchar_t* GetSongLength();
std::wstring CreateSongTickerText();
std::wstring GetInfoText();
int getOutputTimePercentage();
// gets current volume
int curvol = IPC_GETVOLUME(hwnd_winamp);
// gets current panning
int curpan = IPC_GETPANNING(hwnd_winamp);
// gets bitrate
int bitr = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
// gets samplerate
int smpr = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
// gets playback state
// 1 is playing
// 3 is paused
// 0 is stopped
int res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
// gets stereo info
int monoster = SendMessage(hwnd_winamp,WM_WA_IPC,2,IPC_GETINFO);
// gets repeat state
int repeat = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);
// gets shuffle state
int shuffle = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);
int VisMode = 1;
int TimeMode = 0;
LRESULT trackLengthMS = SendMessage(hwnd_winamp, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
int i_trackLengthMS = (int)(trackLengthMS);
BOOL IsPLEditVisible();
BOOL IsEQVisible();
void drawClutterbar(HDC hdc, int x, int y, int width, int height, COLORREF textColor, const std::wstring& text);

// Define the duration in milliseconds for each "second"
const int SECOND_DURATION = 2000;

// Store the timestamp when Winamp is paused
static ULONGLONG pauseStartTime = 0;

int last_y, top, bottom;

int sourceX = 0; // X-coordinate of the top-left corner of the portion
int sourceY = 0; // Y-coordinate of the top-left corner of the portion
int sourceWidth = 0; // Width of the portion
int sourceHeight = 0; // Height of the portion

RECT visRect = {12 * 2, 19 * 2, (13 * 2) + (76 * 2), (20 * 2) + (16 * 2)};
RECT textRect;

static WNDPROC lpOldWinampWndProc; /* Important: Old window procedure pointer */
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MainBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HMENU WinampMenu = NULL;
HWND hMainBox = NULL;
HWND hwndCfg = NULL;
HWND hBitrate = NULL;
HWND hSamplerate = NULL;
HWND hSongTicker = NULL;
HWND hTrackBar = NULL;
HWND hTrackBar2 = NULL;
HWND hTrackBar3 = NULL;

// Declare global variables for SA function pointers
static char* (*export_sa_get)(void) = NULL;
static void (*export_sa_setreq)(int) = NULL;

/* Your menu identifier for the custom menu item in Winamp's main menu */
#define MENUID_MYITEM (46184)

struct Color {
    BYTE r, g, b;
};

Color colors[] = {
    {0, 0, 0},        // color 0 = black
    {24, 33, 41},     // color 1 = grey for dots
    {239, 49, 16},    // color 2 = top of spec
    {206, 41, 16},    // 3
    {214, 90, 0},     // 4
    {214, 102, 0},    // 5
    {214, 115, 0},    // 6
    {198, 123, 8},    // 7
    {222, 165, 24},   // 8
    {214, 181, 33},   // 9
    {189, 222, 41},   // 10
    {148, 222, 33},   // 11
    {41, 206, 16},    // 12
    {50, 190, 16},    // 13
    {57, 181, 16},    // 14
    {49, 156, 8},     // 15
    {41, 148, 0},     // 16
    {24, 132, 8},     // 17 = bottom of spec
    {255, 255, 255},  // 18 = osc 1
    {214, 214, 222},  // 19 = osc 2 (slightly dimmer)
    {181, 189, 189},  // 20 = osc 3
    {160, 170, 175},  // 21 = osc 4
    {148, 156, 165},  // 22 = osc 5
    {150, 150, 150}   // 23 = analyzer peak dots
};

void GetSkinColors() {
    unsigned char* colorBytes = (unsigned char*)SendMessage(hwnd_winamp, WM_WA_IPC, 10, IPC_GET_GENSKINBITMAP);

    if (colorBytes == reinterpret_cast<unsigned char*>(0) || colorBytes == reinterpret_cast<unsigned char*>(1)) {
        // SORRY NOTHING
    }
    else {
        // Extract color values from the response and store them in the colors array
        for (int i = 0; i < 24; ++i) {
            colors[i].r = colorBytes[i * 3];
            colors[i].g = colorBytes[i * 3 + 1];
            colors[i].b = colorBytes[i * 3 + 2];
        }
    }

    // Free the memory allocated by Winamp
    GlobalFree(colorBytes);
}

// Function to convert Color array to COLORREF array
COLORREF* convertToCOLORREF(const Color* colors, int size) {
    COLORREF* colorRefs = new COLORREF[size];
    for (int i = 0; i < size; ++i) {
        colorRefs[i] = RGB(colors[i].r, colors[i].g, colors[i].b);
    }
    return colorRefs;
}

// Function to return the osc_colors array
COLORREF* osccolors(const Color* colors) {
    static Color osc_colors[17];

    osc_colors[0] = colors[21];
    osc_colors[1] = colors[21];
    osc_colors[2] = colors[20];
    osc_colors[3] = colors[20];
    osc_colors[4] = colors[19];
    osc_colors[5] = colors[19];
    osc_colors[6] = colors[18];
    osc_colors[7] = colors[18];
    osc_colors[8] = colors[19];
    osc_colors[9] = colors[19];
    osc_colors[10] = colors[20];
    osc_colors[11] = colors[20];
    osc_colors[12] = colors[21];
    osc_colors[13] = colors[21];
    osc_colors[14] = colors[22];
    osc_colors[15] = colors[22];
    osc_colors[16] = colors[22];

    return convertToCOLORREF(osc_colors, 17);
}

// Function to release memory allocated for COLORREF array
void releaseColorRefs(COLORREF* colorRefs) {
    delete[] colorRefs;
}

RECT createRect(int x, int y, int width, int height) {
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    return rect;
}

// Plugin description
static wchar_t description[] = L"Basic Winamp General Purpose Plugin v1.0";

// Plug-in structure
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER_U,
	(wchar_t *)PLUGIN_TITLE,
	init,
	config,
	quit,
    NULL,
    NULL
};