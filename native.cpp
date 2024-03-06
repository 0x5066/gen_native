// based around https://github.com/WACUP/gen_thinger/blob/main/Example%20plugin/gen_thingerexampleplugin.cpp

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
std::wstring formatTime(int milliseconds);
//const wchar_t* GetSongLength();
std::wstring CreateSongTickerText();
std::wstring GetInfoText();
int getOutputTimePercentage();
int curvol = IPC_GETVOLUME(hwnd_winamp);
int curpan = IPC_GETPANNING(hwnd_winamp);
int bitr = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
int smpr = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
int res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
LRESULT trackLengthMS = SendMessage(hwnd_winamp, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
int i_trackLengthMS = (int)(trackLengthMS);

int last_y, top, bottom;

int sourceX = 0; // X-coordinate of the top-left corner of the portion
int sourceY = 0; // Y-coordinate of the top-left corner of the portion
int sourceWidth = 0; // Width of the portion
int sourceHeight = 0; // Height of the portion

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

// Plugin initialization function
int init() {

    INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&icex);

    WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_TYPE|MIIM_ID;
	mii.dwTypeData	= PLUGIN_CAPTION;
	mii.fType		= MFT_STRING;
	mii.wID			= MENUID_MYITEM;

    InsertMenuItem(WinampMenu, WINAMP_OPTIONS_EQ, FALSE, &mii);
	/* Always remember to adjust "Option" submenu position. */
	/* Note: In Winamp 5.0+ this is unneccesary as it is more intelligent when
	   it comes to menus, but you must do it so it works with older versions. */
	SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);
    // Perform initialization tasks here

    lpOldWinampWndProc = WNDPROC(SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, int(WinampSubclass)));

    return GEN_INIT_SUCCESS; // Indicate successful initialization
}

void quit()
{
    if (IsWindow(hwndCfg)) {
        DestroyWindow(hwndCfg);
        hwndCfg = NULL;
    }
}

void config()
{
	MessageBoxW(plugin.hwndParent,
		PLUGIN_TITLE L"\r\nThis plug-in has nothing to configure!",
		NULL, MB_ICONWARNING);
}

void OpenMyDialog()
{
	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	if (IsWindow(hwndCfg)) {

		/*  Toggle dialog. If it's created, destroy it. */
		DestroyWindow(hwndCfg);
		CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND|MF_UNCHECKED);
		
		/* Code for reshowing it if it's already created.
		ShowWindow(hwndCfg, SW_SHOW);
		SetForegroundWindow(hwndCfg);
		*/
		return;
	}

	hwndCfg = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IDD_MAINWND), 0, MainWndProc);
    if (IsWindow(hwndCfg)) {
        ShowWindow(hwndCfg, SW_SHOW);
        CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND | MF_CHECKED);
    }
    else {
        CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND | MF_UNCHECKED);
    }

}

// Define a global variable for the off-screen buffer
HBITMAP hOffscreenBitmap = NULL;
HDC hOffscreenDC = NULL;

//std::string curvol_str = std::to_string(curvol);

// Callback procedure for the main window
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
    case WM_INITDIALOG:
        // Initialization code here
        WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
        SetWindowTextW(hwnd, CreateSongTickerText().c_str());
        bitr = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
        smpr = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
        res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
        trackLengthMS = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
        i_trackLengthMS = (int)(trackLengthMS);
        hMainBox = GetDlgItem(hwnd, IDC_MAINBOX);
        hBitrate = GetDlgItem(hwnd, IDC_KBPSDISP);
        hSamplerate = GetDlgItem(hwnd, IDC_KHZDISP);
        hSongTicker = GetDlgItem(hwnd, IDC_SONGDISP);
        hTrackBar = GetDlgItem(hwnd, IDC_VOLUMEBAR);
        SendMessage(hTrackBar, TBM_SETRANGE, FALSE, MAKELPARAM(0, 255));
        SendMessage(hTrackBar, TBM_SETPOS, TRUE, curvol);
        hTrackBar2 = GetDlgItem(hwnd, IDC_BALANCEBAR);
        SendMessage(hTrackBar2, TBM_SETRANGE, FALSE, MAKELPARAM(-127, 127));
        SendMessage(hTrackBar2, TBM_SETPOS, TRUE, curpan);
        hTrackBar3 = GetDlgItem(hwnd, IDC_POSBAR);
        SendMessage(hTrackBar3, TBM_SETRANGE, FALSE, MAKELPARAM(0, 128));
        SendMessage(hTrackBar3, TBM_SETPOS, TRUE, getOutputTimePercentage());
        if (res == 1) {
            EnableWindow(hTrackBar3, TRUE);
        } else if (res == 0 || i_trackLengthMS == -1) {
            EnableWindow(hTrackBar3, FALSE);
        }
        //MessageBox(hwnd, curvol_str.c_str(), "", MB_OK);
        // Start the timer here
        SetTimer(hwnd, ID_TIMER, 16, NULL); // 1000 milliseconds interval

        // Specify that we want both spectrum and oscilloscope data
        // Get function pointers from Winamp
        if (!export_sa_get)
            export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
        if (!export_sa_setreq)
            export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
        // Specify that we want both spectrum and oscilloscope data
        export_sa_setreq(1); // Pass 1 to get both spectrum and oscilloscope data
        return TRUE;

    case WM_TIMER: {
        if (wParam == ID_TIMER) {
            //MessageBox(hGWinamp, "This is a test", "Test", MB_OK | MB_ICONINFORMATION);
            if (hMainBox) {
                SetWindowTextW(hwnd, CreateSongTickerText().c_str());
                bitr = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
                smpr = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
                curvol = IPC_GETVOLUME(hwnd_winamp);
                curpan = IPC_GETPANNING(hwnd_winamp);
                res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
                trackLengthMS = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
                i_trackLengthMS = (int)(trackLengthMS);
                SendMessage(hTrackBar, TBM_SETPOS, TRUE, curvol);
                SendMessage(hTrackBar2, TBM_SETPOS, TRUE, curpan);
                SendMessage(hTrackBar3, TBM_SETPOS, TRUE, getOutputTimePercentage());
                if (res == 1) {
                    EnableWindow(hTrackBar3, TRUE);
                } else if (res == 0 || i_trackLengthMS == -1) {
                    EnableWindow(hTrackBar3, FALSE);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return TRUE;
    }

    case WM_COMMAND:
        // Handle command messages here
        break;

    case WM_CLOSE:
        // Close the window
        CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND | MF_UNCHECKED);
        DestroyWindow(hwnd);
        break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hMainBox, &ps);

            RECT rc;
            GetClientRect(hMainBox, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            HDC hdcBuffer = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);
            char* sadata;

            // Draw onto the off-screen buffer
            // Fill the background with white
            HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            int fontSize = 34; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            RECT textRect = { 10, 3, width - 10, height - 10 }; // Adjust the coordinates and size as needed
            const std::wstring infoText = GetInfoText();
            DrawTextW(hdcBuffer, infoText.c_str(), infoText .size(), &textRect, DT_RIGHT | DT_TOP);

            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            // Get SA data
            sadata = export_sa_get();

            for (int y = 19; y < 37; y++) {
                SetPixel(hdcBuffer, 11 * 2, y * 2, RGB(0,0,0));
            }

            for (int x = 11; x < 89; x++) {
                SetPixel(hdcBuffer, x * 2, 37 * 2, RGB(0,0,0));
            }

            /* MoveToEx(hdcBuffer, 22, 38, NULL);
            LineTo(hdcBuffer, 22, 76); */

            /* MoveToEx(hdcBuffer, 22, 76, NULL);
            LineTo(hdcBuffer, 180, 76); */

            // Load the bitmap from the resource
            HBITMAP hResBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PLAYPAUS)); // IDB_PLAYPAUS is the ID of your bitmap resource

            if (hResBitmap)
            {
                // Get the dimensions of the bitmap
                BITMAP bmpInfo;
                GetObject(hResBitmap, sizeof(BITMAP), &bmpInfo);

                // Define variables for source coordinates and dimensions
                int sourceX, sourceY, sourceWidth, sourceHeight;

                // Adjust source coordinates and dimensions based on the value of 'res'
                if (res == 1) {
                    sourceX = 0;
                    sourceY = 0;
                    sourceWidth = 18;
                    sourceHeight = 18;
                } else if (res == 0) {
                    sourceX = 36;
                    sourceY = 0;
                    sourceWidth = 18;
                    sourceHeight = 18;
                } else if (res == 3) {
                    sourceX = 18;
                    sourceY = 0;
                    sourceWidth = 18;
                    sourceHeight = 18;
                } else {
                    // Default values if 'res' is not recognized
                    sourceX = 0;
                    sourceY = 0;
                    sourceWidth = bmpInfo.bmWidth; // Entire width of the bitmap
                    sourceHeight = bmpInfo.bmHeight; // Entire height of the bitmap
                }

                // Create a compatible DC for the bitmap
                HDC hdcBitmap = CreateCompatibleDC(hdcBuffer);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBitmap, hResBitmap);

                // Draw the first portion onto the off-screen buffer at the desired location
                BitBlt(hdcBuffer, 30, 10, sourceWidth, sourceHeight, hdcBitmap, sourceX, sourceY, SRCCOPY);

                if (res == 1) {
                    // Draw the second portion onto the off-screen buffer at the desired location
                    BitBlt(hdcBuffer, 26, 10, 6, 18, hdcBitmap, 72, 0, SRCCOPY);
                }

                // Clean up
                SelectObject(hdcBitmap, hOldBitmap);
                DeleteDC(hdcBitmap);
                DeleteObject(hResBitmap);
            }

            if (sadata) {

                for (int x = 0; x < 75; x++) {
                    signed char y = sadata[x + 75];
                    int intValue = y + 6;
                    intValue = intValue < 0 ? 0 : (intValue > 16 - 1 ? 16 - 1 : intValue);

                    // Calculate the endpoint of the line based on intValue
                    int endpointY = (intValue * 2) + 40; // Adjusted for starting at y=42

                    // grab the first point of sadata so that we dont have a weird
                    // line that's just static
                    if (x == 0) {
                        MoveToEx(hdcBuffer, 26, 40 + (intValue * 2), NULL);
                    } else {
                        LineTo(hdcBuffer, (x * 2) + 26, endpointY);
                    }
                }
            }

                // Draw oscilloscope data
/*          if (sadata) {
                for (int x = 0; x < 75; x++) {
                    signed char y = sadata[x + 75];
                    int intValue = y + 6;
                    intValue = intValue < 0 ? 0 : (intValue > 16 - 1 ? 16 - 1 : intValue);

                    if (x == 0) {
                        last_y = intValue; // Use intValue directly
                    }

                    top = intValue; // Use intValue directly
                    bottom = last_y;
                    last_y = intValue;

                    if (bottom < top) {
                        int temp = bottom;
                        bottom = top;
                        top = temp + 1;
                    }

                        for (int dy = top; dy <= bottom; dy++) {
                            RECT rect = {(x * 2) + 26, (dy * 2) + 40, (x * 2) + 26 + 2, (dy * 2) + 40 + 2}; // Rectangles of width 2px and height 2px
                            FillRect(hdcBuffer, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)); // Draw the rectangle
                        }
                    }
                } */
            
            // Copy the off-screen buffer to the screen
            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hMainBox, &ps);

            // begin anew with bitrate drawing
            hdc = BeginPaint(hBitrate, &ps);
            GetClientRect(hBitrate, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            hdcBuffer = CreateCompatibleDC(hdc);
            hBitmap = CreateCompatibleBitmap(hdc, width, height);
            hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            fontSize = 13; // Change this to the desired font size
            hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            textRect = { 0, 3, width, height }; // Adjust the coordinates and size as needed
            std::string bitr_str = std::to_string(bitr);
            DrawTextA(hdcBuffer, bitr_str.c_str(), -1, &textRect, DT_CENTER | DT_TOP);

            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hBitrate, &ps);

            // begin anew with samplerate drawing
            hdc = BeginPaint(hSamplerate, &ps);
            GetClientRect(hSamplerate, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            hdcBuffer = CreateCompatibleDC(hdc);
            hBitmap = CreateCompatibleBitmap(hdc, width, height);
            hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            fontSize = 13; // Change this to the desired font size
            hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            textRect = { 0, 3, width, height }; // Adjust the coordinates and size as needed
            std::string smpr_str = std::to_string(smpr);
            DrawTextA(hdcBuffer, smpr_str.c_str(), -1, &textRect, DT_CENTER | DT_TOP);

            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hSamplerate, &ps);

            // begin anew with songticker drawing
            hdc = BeginPaint(hSongTicker, &ps);
            GetClientRect(hSongTicker, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            hdcBuffer = CreateCompatibleDC(hdc);
            hBitmap = CreateCompatibleBitmap(hdc, width, height);
            hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            fontSize = 13; // Change this to the desired font size
            hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            textRect = { 6, 3, width - 6, height }; // Adjust the coordinates and size as needed
            DrawTextW(hdcBuffer, CreateSongTickerText().c_str(), -1, &textRect, DT_LEFT | DT_TOP);

            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hSongTicker, &ps);
            return 0;
        }

        case WM_HSCROLL: {
            // Check if the message is from the TrackBar
            if ((HWND)lParam == hTrackBar) {
                // Get the current position of the TrackBar
                int position = SendMessage(hTrackBar, TBM_GETPOS, 0, 0);

                // Set the volume of Winamp using IPC_SETVOLUME
                SendMessage(hwnd_winamp, WM_WA_IPC, position, IPC_SETVOLUME);
            }
            if ((HWND)lParam == hTrackBar2) {
                // Get the current position of the TrackBar
                int position = SendMessage(hTrackBar2, TBM_GETPOS, 0, 0);

                // Set the volume of Winamp using IPC_SETVOLUME
                SendMessage(hwnd_winamp, WM_WA_IPC, position, IPC_SETBALANCE);
            }
            break;
        }

        default:
            return FALSE;
    }
    return TRUE;
}

// Callback procedure for the main box control
/* LRESULT CALLBACK MainBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int intValue; // Variable to hold intValue
    int last_y = 0;
	int top, bottom;

    //MessageBox(hGWinamp, "This is a test", "Test", MB_OK | MB_ICONINFORMATION);

    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, ID_TIMER, 16, NULL); // 1000 milliseconds interval
            return 0;

        case WM_TIMER: {
            if (wParam == ID_TIMER) {
                //MessageBox(hGWinamp, "This is a test", "Test", MB_OK | MB_ICONINFORMATION);
                if (hMainBox) {
                    //InvalidateRect(hwnd, NULL, TRUE);
                    UpdateWindow(hwnd);
                }
            }
            return TRUE;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Get the client area dimensions
            RECT rc;
            GetClientRect(hMainBox, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // Fill the client area with a background color
            HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdc, &rc, hBrushBg);
                char* sadata;
            // Specify that we want both spectrum and oscilloscope data
            // Get function pointers from Winamp
            if (!export_sa_get)
                export_sa_get = (char* (*)(void))SendMessage(hGWinamp, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
            if (!export_sa_setreq)
                export_sa_setreq = (void (*)(int))SendMessage(hGWinamp, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
            // Specify that we want both spectrum and oscilloscope data
            export_sa_setreq(1); // Pass 1 to get both spectrum and oscilloscope data

            sadata = export_sa_get();

            // Get SA data


            if (sadata) {

                // Draw oscilloscope data
                for (int x = 0; x < 75; x++) {
                    signed char y = sadata[x + 75];
                    int intValue = y + 16;

                    if (x == 0) {
                        last_y = intValue; // Use intValue directly
                    }

                    top = intValue; // Use intValue directly
                    bottom = last_y;
                    last_y = intValue;

                    if (bottom < top) {
                        int temp = bottom;
                        bottom = top;
                        top = temp + 1;
                    }

                    for (int dy = top; dy <= bottom; dy++) {
                        SetPixel(hdc, x, dy, RGB(0, 0, 0));
                    }
                }
            }
            
            DeleteObject(hBrushBg);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        default:
            // Call the default window procedure for other messages
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
} */

LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(wParam)==MENUID_MYITEM && HIWORD(wParam) == 0)
		{
			OpenMyDialog();
			return 0;
		}
		break;
	/* Also handle WM_SYSCOMMAND if people are selectng the item through
	   the Winamp submenu in system menu. */
	case WM_SYSCOMMAND:
		if (wParam == MENUID_MYITEM)
		{
			OpenMyDialog();
			return 0;
		}
		break;
	}


	/* Call previous window procedure */
	return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
}

std::wstring formatTime(int milliseconds) {
    // Convert milliseconds to seconds
    int totalSeconds = milliseconds / 1000;

    // Extract minutes and seconds
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    // Format as MM:SS
    std::wstringstream ss;
    ss << std::setw(2) << std::setfill(L'0') << minutes << L":"
       << std::setw(2) << std::setfill(L'0') << seconds;

    return ss.str();
}

std::wstring GetSongLength() {
    // Send the WM_WA_IPC message to get the current output time
    LRESULT waOutputTime2 = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert the time to MM:SS format
    std::wstring outputTimeString2 = formatTime(static_cast<int>(waOutputTime2));

    // Duplicate the string to return a const char*
    return outputTimeString2;
}

std::wstring CreateSongTickerText() {
    int pos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);

    wchar_t* titleStr = (wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_TITLE);
    
    //const wchar_t* getSongTime = GetSongLength();
    // Send the WM_WA_IPC message to get the current output time
    LRESULT waOutputTime2 = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert the time to MM:SS format
    std::wstring getSongTime = formatTime(static_cast<int>(waOutputTime2));

    // Combine the strings
    return std::to_wstring(pos + 1) + L". " + titleStr + L" (" + getSongTime + L")";
}

std::wstring GetInfoText() {
    // Send the WM_WA_IPC message to get the current output time
    LRESULT waOutputTime = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
    //LRESULT waOutputTime2 = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert the time to MM:SS format
    std::wstring outputTimeString = formatTime(static_cast<int>(waOutputTime));
    //std::string outputTimeString2 = formatTime(static_cast<int>(waOutputTime2));

    // Duplicate the string to return a const wchar_t*
    return outputTimeString;
}

int getOutputTimePercentage() {
    // Get the output time of the currently playing track in milliseconds
    LRESULT waOutputTime = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);

    // Get the total length of the currently playing track in milliseconds
    LRESULT waOutputTime2 = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert milliseconds to seconds
    int outputTimeSeconds = (int)(waOutputTime / 1000);
    int totalTrackLengthSeconds = (int)(waOutputTime2 / 1000);

    // Calculate the percentage completion
    if (outputTimeSeconds >= 0 && totalTrackLengthSeconds > 0) {
        int percentage = (int)((double)outputTimeSeconds / totalTrackLengthSeconds * 128);
        if (percentage > 128) {
            return 128; // Cap the percentage at 256
        } else {
            return percentage;
        }
    } else {
        return 0; // Return 0 if there's no track playing or if track length is not available
    }
}

// Plugin getter function
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}