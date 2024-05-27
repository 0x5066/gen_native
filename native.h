#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <commctrl.h>
#include <shlwapi.h>
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
#define ID_TIMER2 2

HWND hwnd_winamp = FindWindow("Winamp v1.x", NULL); // find winamp

prefsDlgRecW *prefsRec = NULL;

// Function declarations
void config(void);
void quit(void);
int init(void);
void OpenMyDialog();
void DrawMainBox(HWND hMainBox, int res);
void DrawBitrate(HWND hBitrate, int res, int bitr);
void DrawSamplerate(HWND hSamplerate, int res, int smpr);
void DrawSongTicker(HWND hSongTicker);
void config_getinifnW(wchar_t* ini_file);
void config_read();
void config_write();
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
// gets sync status
int sync = SendMessage(hwnd_winamp,WM_WA_IPC,6,IPC_GETINFO);
int VisMode = 0;
int TimeMode = 0;
LRESULT trackLengthMS = SendMessage(hwnd_winamp, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
int i_trackLengthMS = (int)(trackLengthMS);
BOOL IsPLEditVisible();
BOOL IsEQVisible();
void drawClutterbar(HDC hdc, int x, int y, int width, int height, const std::wstring& text);
float GetWindowDPI(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    float dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    return dpiX; // Assuming X and Y DPI are the same for most cases
}
bool modernskinyesno();
bool modern;

int rectoffsetbyone;

float DPIscale = 96.0f;

// Define the duration in milliseconds for each "second"
const int SECOND_DURATION = 2000;

// Store the timestamp when Winamp is paused
static ULONGLONG pauseStartTime = 0;

int last_y, top, bottom;

int sourceX = 0; // X-coordinate of the top-left corner of the portion
int sourceY = 0; // Y-coordinate of the top-left corner of the portion
int sourceWidth = 0; // Width of the portion
int sourceHeight = 0; // Height of the portion

bool modernsize = false;
int vis_size = 75;
bool native = false;
bool peaksatzero = false;
int peakthreshold = 15;
bool drawvisgrid = false;

int bar_falloff[5] = {3, 6, 12, 16, 32};
float peak_falloff[5] = {1.05f, 1.1f, 1.2f, 1.4f, 1.6f};

int config_safalloff = 3;
int config_sa_peak_falloff = 2;

wchar_t specdraw[] = L"normal";
wchar_t bandwidth[] = L"thick";
wchar_t oscstyle[] = L"lines";
wchar_t vudraw[] = L"normal";

bool peaks = true;

RECT textRect;

static WNDPROC lpOldWinampWndProc; /* Important: Old window procedure pointer */
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);

HMENU WinampMenu = NULL;
HWND hMainBox = NULL;
HWND hwndCfg = NULL;
HWND hBitrate = NULL;
HWND hSamplerate = NULL;
HWND hSongTicker = NULL;
HWND hTrackBar = NULL;
HWND hTrackBar2 = NULL;
HWND hTrackBar3 = NULL;

HWND hTestBox = NULL;
HWND hTabControl; // Handle to the tab control
HWND hTab1, hTab2; // Handles to the child windows of each tab
HWND hSAFalloffBar;
HWND hPFalloffBar;
HWND hVisCombo;

// Define the RECT with DPI scaling applied
//UINT newDPI = GetDpiFromDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
RECT visRect = {
    MulDiv(12 * 2, GetWindowDPI(hwndCfg), 96),    // Left coordinate scaled according to DPI
    MulDiv(19 * 2, GetWindowDPI(hwndCfg), 96),    // Top coordinate scaled according to DPI
    MulDiv((13 * 2 + 76 * 2), GetWindowDPI(hwndCfg), 96),  // Right coordinate scaled according to DPI
    MulDiv((20 * 2 + 16 * 2), GetWindowDPI(hwndCfg), 96)  // Bottom coordinate scaled according to DPI
};

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

// Define osccolors array outside of any function
Color osccolors[17];

// Initialize the osccolors array
void InitializeOscColors(Color colors[]) {
    osccolors[0] = colors[21];
    osccolors[1] = colors[21];
    osccolors[2] = colors[20];
    osccolors[3] = colors[20];
    osccolors[4] = colors[19];
    osccolors[5] = colors[19];
    osccolors[6] = colors[18];
    osccolors[7] = colors[18];
    osccolors[8] = colors[19];
    osccolors[9] = colors[19];
    osccolors[10] = colors[20];
    osccolors[11] = colors[20];
    osccolors[12] = colors[21];
    osccolors[13] = colors[21];
    osccolors[14] = colors[22];
    osccolors[15] = colors[22];
    osccolors[16] = colors[22];
}

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
    
    // Update osccolors with the new skin colors
    InitializeOscColors(colors);
}

Color BlendColors(const Color& color1, const Color& color2, float ratio) {
    // Blend the colors using a weighted average
    Color blendedColor;
    blendedColor.r = static_cast<unsigned char>((1.0f - ratio) * color1.r + ratio * color2.r);
    blendedColor.g = static_cast<unsigned char>((1.0f - ratio) * color1.g + ratio * color2.g);
    blendedColor.b = static_cast<unsigned char>((1.0f - ratio) * color1.b + ratio * color2.b);
    return blendedColor;
}

// Helper function to convert DWORD color to Color structure
Color ConvertDWORDToColor(DWORD colorValue) {
    Color color;
    color.r = GetRValue(colorValue);
    color.g = GetGValue(colorValue);
    color.b = GetBValue(colorValue);
    return color;
}

void BlendAndWriteColors(const Color& color1, const Color& color2, const Color& color3, const Color& color4, Color* colors) {
    // Blend the colors and write them to the array from the 3rd to the 17th positions
    for (int i = 2; i < 18; ++i) {
        float ratio = static_cast<float>(i - 2) / 15.0f; // Calculate blending ratio
        colors[i] = BlendColors(color1, color2, ratio);
    }
    
    // Blend the additional colors and write them to the array from the 18th to the 22nd positions
    for (int i = 18; i < 23; ++i) {
        float ratio = static_cast<float>(i - 18) / 5.0f; // Calculate blending ratio
        colors[i] = BlendColors(color3, color4, ratio);
    }

    colors[23] = BlendColors(ConvertDWORDToColor(GetSysColor(COLOR_3DFACE)), color4, 0.5f);
    colors[0] = ConvertDWORDToColor(GetSysColor(COLOR_WINDOW));
    colors[1] = BlendColors(ConvertDWORDToColor(GetSysColor(COLOR_3DFACE)), color4, 0.125f);
}

// Function to convert Color array to COLORREF array
COLORREF* convertToCOLORREF(const Color* colors, int size) {
    COLORREF* colorRefs = new COLORREF[size];
    for (int i = 0; i < size; ++i) {
        colorRefs[i] = RGB(colors[i].r, colors[i].g, colors[i].b);
    }
    return colorRefs;
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

void DumpColorsToBMP(const Color* colors, int width, int height, const char* filename) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    // File header
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * sizeof(RGBQUAD);
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Info header
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    // Write to file
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&bfh), sizeof(BITMAPFILEHEADER));
        file.write(reinterpret_cast<const char*>(&bih), sizeof(BITMAPINFOHEADER));

        // Write color data
        for (int i = height - 1; i >= 0; --i) {
            for (int j = 0; j < width; ++j) {
                RGBQUAD rgb;
                rgb.rgbBlue = colors[i * width + j].b;
                rgb.rgbGreen = colors[i * width + j].g;
                rgb.rgbRed = colors[i * width + j].r;
                file.write(reinterpret_cast<const char*>(&rgb), sizeof(RGBQUAD));
            }
        }

        file.close();
    }
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

// hacky way to invalidate and update my hwnds
// it feels like i shouldnt need this, but i do???
void InvalidateHWND(HWND hwnd1, HWND hwnd) {
    RECT hwnd1Rect;
    GetWindowRect(hwnd1, &hwnd1Rect); // Get screen coordinates of hMainBox

    // Convert screen coordinates to client coordinates of the parent window
    POINT topLeft = { hwnd1Rect.left, hwnd1Rect.top };
    POINT bottomRight = { hwnd1Rect.right, hwnd1Rect.bottom };
    ScreenToClient(hwnd, &topLeft);
    ScreenToClient(hwnd, &bottomRight);

    // Create a RECT structure from the client coordinates
    RECT clientRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };

    // Invalidate the region
    InvalidateRect(hwnd, &clientRect, FALSE);
}

bool modernskinyesno(){
    bool yesno;
    WCHAR skinname_buffer[MAX_PATH];
    SendMessage(hwnd_winamp, WM_WA_IPC, (WPARAM)skinname_buffer, IPC_GETSKINW);
    // Check if the skin.xml file exists
    WCHAR skinXmlPath[MAX_PATH];
    wcscpy_s(skinXmlPath, MAX_PATH, skinname_buffer);
    PathAppendW(skinXmlPath, L"skin.xml");
    if (PathFileExistsW(skinXmlPath)) {
        yesno = TRUE;
    } else {
        yesno = FALSE;
    }
    return yesno;
}