// based around https://github.com/WACUP/gen_thinger/blob/main/Example%20plugin/gen_thingerexampleplugin.cpp

#include <windows.h>
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

// Function declarations
void config(void);
void quit(void);
int init(void);
void OpenMyDialog();
std::string formatTime(int milliseconds);
const char* GetSongLength(HWND winampWindow);
std::string CreateSongTickerText(HWND winampWindow);
const char* GetInfoText(HWND winampWindow);

static WNDPROC lpOldWinampWndProc; /* Important: Old window procedure pointer */
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MainBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HMENU WinampMenu;
HWND hMainBox = NULL;
HWND hGWinamp = NULL;

/* Your menu identifier for the custom menu item in Winamp's main menu */
#define MENUID_MYITEM (46184)

// Plugin description
static wchar_t description[] = L"Basic Winamp General Purpose Plugin v1.0";

// Plug-in structure
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER_U,
	PLUGIN_TITLE,
	init,
	config,
	quit,
    NULL,
    NULL
};

// Plugin initialization function
int init() {

    WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
    hGWinamp = plugin.hwndParent;

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

	PostMessage(plugin.hwndParent, WM_WA_IPC, 666, IPC_GETVERSION);

    return GEN_INIT_SUCCESS; // Indicate successful initialization
}

void quit()
{

}

void config()
{
	MessageBoxW(plugin.hwndParent,
		TEXT(PLUGIN_TITLE L"\r\nThis plug-in has nothing to configure!"),
		NULL, MB_ICONWARNING);
}

void OpenMyDialog()
{
	static HWND hwndCfg=NULL;
	MSG msg;

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

	ShowWindow(hwndCfg, SW_SHOW);
	CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND|MF_CHECKED);

/* 	while (IsWindow(hwndCfg) && IsWindow(plugin.hwndParent) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hwndCfg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	hwndCfg = NULL; */

	/* Modal dialog box code
	if (DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(IDD_MYDIALOG), hwnd, ConfigDlgProc)) {
		ShowWindow(ews->me, config_enabled?SW_SHOW:SW_HIDE);
	}
	*/

}

// Timer ID
#define ID_TIMER 1

// Define a global variable for the off-screen buffer
HBITMAP hOffscreenBitmap = NULL;
HDC hOffscreenDC = NULL;

// Callback procedure for the main window
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HMENU WinampMenu; // Define this globally or outside this function
    static HWND hMainBox; // Define this globally or outside this function
    static char* (*export_sa_get)(void) = NULL; // Function pointer for getting SA data
    static void (*export_sa_setreq)(int) = NULL; // Function pointer for setting SA data request
    static int i = 0;
    int intValue;
    int last_y = 0;
    int top, bottom;
    const std::string combinedText = CreateSongTickerText(plugin.hwndParent);
    const char* infoText = GetInfoText(plugin.hwndParent);

    switch (msg) {
    case WM_INITDIALOG:
        // Initialization code here
        WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
        SetWindowText(hwnd, combinedText.c_str());
        hMainBox = GetDlgItem(hwnd, IDC_MAINBOX);
        // Start the timer here
        SetTimer(hwnd, ID_TIMER, 16, NULL); // 1000 milliseconds interval
        return TRUE;

    case WM_TIMER: {
        if (wParam == ID_TIMER) {
            //MessageBox(hGWinamp, "This is a test", "Test", MB_OK | MB_ICONINFORMATION);
            if (hMainBox) {
                SetWindowText(hwnd, combinedText.c_str());
                RECT updateRect;
                GetClientRect(hMainBox, &updateRect);
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
        EndDialog(hwnd, 0);
        break;

    case WM_CREATE:
        return 0;

    /* case WM_ERASEBKGND:
        return 1; // Indicate that we've handled the background erasing */

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
            int fontSize = 36; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            RECT textRect = { 10, 0, width - 10, height - 10 }; // Adjust the coordinates and size as needed
            const char* infoText = GetInfoText(plugin.hwndParent);
            DrawTextA(hdcBuffer, infoText, -1, &textRect, DT_RIGHT | DT_TOP);

            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            // Specify that we want both spectrum and oscilloscope data
            // Get function pointers from Winamp
            if (!export_sa_get)
                export_sa_get = (char* (*)(void))SendMessage(hGWinamp, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
            if (!export_sa_setreq)
                export_sa_setreq = (void (*)(int))SendMessage(hGWinamp, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
            // Specify that we want both spectrum and oscilloscope data
            export_sa_setreq(1); // Pass 1 to get both spectrum and oscilloscope data
            // Get SA data
            sadata = export_sa_get();

                for (int x = 0; x < 75; x++) {
                    signed char y = sadata[x + 75];
                    int intValue = y + 6;
                    intValue = intValue < 0 ? 0 : (intValue > 16 - 1 ? 16 - 1 : intValue);

                    // Calculate the endpoint of the line based on intValue
                    int endpointY = (intValue * 2) + 40; // Adjusted for starting at y=42

                    // grab the first point of sadata so that we dont have a weird
                    // line that's just static
                    if (x == 0) {
                        MoveToEx(hdcBuffer, 26, 28 + (intValue * 2) + 12, NULL);
                    } else {
                        LineTo(hdcBuffer, (x * 2) + 26, endpointY);
                    }
                }

                // Draw oscilloscope data
/*                for (int x = 0; x < 75; x++) {
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
                        RECT rect = {(x * 2) + 28, (dy * 2) + 42, (x * 2) + 28 + 2, (dy * 2) + 42 + 2}; // Rectangles of width 2px and height 2px
                        FillRect(hdcBuffer, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)); // Draw the rectangle
                    }
                }*/
            
                    // Copy the off-screen buffer to the screen
            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hMainBox, &ps);
            return 0;
        }

        default:
            return FALSE;
    }
    return TRUE;
}

// Declare global variables for SA function pointers
static char* (*export_sa_get)(void) = NULL;
static void (*export_sa_setreq)(int) = NULL;

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

std::string formatTime(int milliseconds) {
    // Convert milliseconds to seconds
    int totalSeconds = milliseconds / 1000;

    // Extract minutes and seconds
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    // Format as MM:SS
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << minutes << ":"
       << std::setw(2) << std::setfill('0') << seconds;

    return ss.str();
}

const char* GetSongLength(HWND winampWindow) {
    // Send the WM_WA_IPC message to get the current output time
    LRESULT waOutputTime2 = SendMessage(winampWindow, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert the time to MM:SS format
    std::string outputTimeString2 = formatTime(static_cast<int>(waOutputTime2));

    // Duplicate the string to return a const char*
    char* infoText = strdup(outputTimeString2.c_str());
    return infoText;
}

std::string CreateSongTickerText(HWND winampWindow) {
    int pos = SendMessage(winampWindow, WM_WA_IPC, 0, IPC_GETLISTPOS);

    wchar_t* title = (wchar_t*)SendMessage(winampWindow, WM_WA_IPC, 0, IPC_GET_PLAYING_TITLE);
    
    // Convert wchar_t* to std::wstring and then to std::string
    std::wstring titleWstr(title);
    std::string titleStr(titleWstr.begin(), titleWstr.end());

    const char* getSongTime = GetSongLength(winampWindow);

    // Combine the strings
    return std::to_string(pos + 1) + ". " + titleStr + " (" + getSongTime + ")";
}

const char* GetInfoText(HWND winampWindow) {
    // Send the WM_WA_IPC message to get the current output time
    LRESULT waOutputTime = SendMessage(winampWindow, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
    //LRESULT waOutputTime2 = SendMessage(winampWindow, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert the time to MM:SS format
    std::string outputTimeString = formatTime(static_cast<int>(waOutputTime));
    //std::string outputTimeString2 = formatTime(static_cast<int>(waOutputTime2));

    // Combine the strings
    std::string combinedOutputTime = outputTimeString;

    // Duplicate the string to return a const char*
    return _strdup(combinedOutputTime.c_str());
}

// Plugin getter function
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}