// based around https://github.com/WACUP/gen_thinger/blob/main/Example%20plugin/gen_thingerexampleplugin.cpp
#include "native.h"

// Plugin initialization function
int init() {

    INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&icex);

    COLORREF* oscColors = osccolors(colors);

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

        /* Toggle dialog. If it's created, destroy it. */
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
        // Get the position and size of plugin.hwndParent
        RECT parentRect;
        GetWindowRect(plugin.hwndParent, &parentRect);

        // Set the position of the dialog window
        SetWindowPos(hwndCfg, HWND_TOP, parentRect.left, parentRect.top, 0, 0, SWP_NOSIZE);
        
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

int ret = 0; //debug value
HBRUSH hBrushRed = CreateSolidBrush(RGB(255, 0, 0));

// Callback procedure for the main window
INT_PTR CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    //std::string message = "The value of monoster is: " + std::to_string(ret);

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
        monoster = SendMessage(hwnd_winamp,WM_WA_IPC,2,IPC_GETINFO);
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
        repeat=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);
        CheckDlgButton(hwnd, IDC_REPEAT, repeat ? BST_CHECKED : BST_UNCHECKED);
        shuffle=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);
        CheckDlgButton(hwnd, IDC_SHUFFLE, shuffle ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_EQ, IsEQVisible() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_PL, IsPLEditVisible() ? BST_CHECKED : BST_UNCHECKED);
        if (res == 1) {
            EnableWindow(hTrackBar3, TRUE);
        } else if (res == 0 || i_trackLengthMS == -1) {
            EnableWindow(hTrackBar3, FALSE);
        }

        if (monoster == 1) {
            EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MONO), TRUE);
            ret = 1;
        } if (monoster == 2) {
            EnableWindow(GetDlgItem(hwnd, IDC_STEREO), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
            ret = 2;
        } if (monoster == 0 || monoster == -1) {
            EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
            ret = 3;
        } if (res == 0) {
            EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
            ret = 4;
        }
        //MessageBox(hwnd, curvol_str.c_str(), "", MB_OK);
        // Start the timer here
        SetTimer(hwnd, ID_TIMER, 16, NULL);

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
            if (hMainBox) {
                SetWindowTextW(hwnd, CreateSongTickerText().c_str());
                bitr = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETINFO);
                smpr = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINFO);
                curvol = IPC_GETVOLUME(hwnd_winamp);
                curpan = IPC_GETPANNING(hwnd_winamp);
                res = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING);
                trackLengthMS = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);
                i_trackLengthMS = (int)(trackLengthMS);
                monoster = SendMessage(hwnd_winamp,WM_WA_IPC,2,IPC_GETINFO);
                repeat=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_REPEAT);
                CheckDlgButton(hwnd, IDC_REPEAT, repeat ? BST_CHECKED : BST_UNCHECKED);
                shuffle=SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GET_SHUFFLE);
                CheckDlgButton(hwnd, IDC_SHUFFLE, shuffle ? BST_CHECKED : BST_UNCHECKED);
                SendMessage(hTrackBar, TBM_SETPOS, TRUE, curvol);
                SendMessage(hTrackBar2, TBM_SETPOS, TRUE, curpan);
                SendMessage(hTrackBar3, TBM_SETPOS, TRUE, getOutputTimePercentage());
                CheckDlgButton(hwnd, IDC_EQ, IsEQVisible() ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwnd, IDC_PL, IsPLEditVisible() ? BST_CHECKED : BST_UNCHECKED);
                if (res == 1) {
                    EnableWindow(hTrackBar3, TRUE);
                } else if (res == 0 || i_trackLengthMS == -1) {
                    EnableWindow(hTrackBar3, FALSE);
                }

                if (monoster == 1) {
                    EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_MONO), TRUE);
                    ret = 1;
                } if (monoster == 2) {
                    EnableWindow(GetDlgItem(hwnd, IDC_STEREO), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
                    ret = 2;
                } if (monoster == 0 || monoster == -1) {
                    EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
                    ret = 3;
                } if (res == 0) {
                    EnableWindow(GetDlgItem(hwnd, IDC_STEREO), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_MONO), FALSE);
                    ret = 4;
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PREV:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON1,0);
                return TRUE;
            }
            break;
        case IDC_PLAY:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON2,0);
                return TRUE;
            }
            break;
        case IDC_STOP:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON4,0);
                return TRUE;
            }
            break;
        case IDC_PAUSE:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON3,0);
                return TRUE;
            }
            break;
        case IDC_NEXT:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON5,0);
                return TRUE;
            }
            break;
        case IDC_EJECT:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_FILE_PLAY,0);
                return TRUE;
            }
            break;
        case IDC_REPEAT:
        // there's a better way to do this, right?
        // check WM_TIMER CheckDlgButton(..) because holy shit
        // this is just nasty
        // if anyone knows a better way plz send a PR
            if (HIWORD(wParam) == BN_CLICKED) {
                int setrep = 0;
                if (repeat == 1) {
                    setrep = 0;
                } else {
                    setrep = 1;
                }

                SendMessage(hwnd_winamp, WM_WA_IPC, setrep, IPC_SET_REPEAT);
                return TRUE;
            }
            break;
        case IDC_SHUFFLE:
        // there's a better way to do this, right?
        // check WM_TIMER CheckDlgButton(..) because holy shit
        // this is just nasty
        // if anyone knows a better way plz send a PR
            if (HIWORD(wParam) == BN_CLICKED) {
                int setshuf = 0;
                if (shuffle == 1) {
                    setshuf = 0;
                } else {
                    setshuf = 1;
                }

                SendMessage(hwnd_winamp, WM_WA_IPC, setshuf, IPC_SET_SHUFFLE);
                return TRUE;
            }
            break;
        case IDC_EQ:
            if (HIWORD(wParam) == BN_CLICKED) {
                // Toggle the visibility state of the Equalizer window
                SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_EQ, 0);

                // Update the menu item state based on the new visibility state
                BOOL isVisible = IsEQVisible();
                CheckMenuItem(WinampMenu, WINAMP_OPTIONS_EQ, isVisible ? MF_CHECKED : MF_UNCHECKED);

                // Update the check button state based on the new visibility state
                CheckDlgButton(hwnd, IDC_EQ, isVisible ? BST_CHECKED : BST_UNCHECKED);

                std::string curvol_str = std::to_string(isVisible);
                //MessageBox(plugin.hwndParent, curvol_str.c_str(), "Test", MB_OK | MB_ICONINFORMATION);

                return TRUE;
            }

        case IDC_PL:
            if (HIWORD(wParam) == BN_CLICKED) {
                // Toggle the visibility state of the Playlist window
                SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_PLEDIT, 0);

                // Update the menu item state based on the new visibility state
                BOOL isVisible = IsPLEditVisible();
                CheckMenuItem(WinampMenu, WINAMP_OPTIONS_PLEDIT, isVisible ? MF_CHECKED : MF_UNCHECKED);

                // Update the check button state based on the new visibility state
                CheckDlgButton(hwnd, IDC_PL, isVisible ? BST_CHECKED : BST_UNCHECKED);

                std::string curvol_str = std::to_string(isVisible);
                //MessageBox(plugin.hwndParent, curvol_str.c_str(), "Test", MB_OK | MB_ICONINFORMATION);

                return TRUE;
            }
        }

    case WM_CLOSE:
        // Close the window
        CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND | MF_UNCHECKED);
        DestroyWindow(hwnd);
        break;

    case WM_PAINT: {
        DrawMainBox(hMainBox, res);
        DrawBitrate(hBitrate, res, bitr);
        DrawSamplerate(hSamplerate, res, smpr);
        DrawSongTicker(hSongTicker);
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

        case WM_LBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hMainBox, &pt);
            if (PtInRect(&visRect, pt)) {
                // Switch to the next visualization mode
                VisMode++;
                if (VisMode > 3) // Wrap around if the mode exceeds 3
                    VisMode = 1;
            }
            if (PtInRect(&textRect, pt)) {
                TimeMode++;
                if (TimeMode > 1)
                    TimeMode = 0;
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            // Get the distance that the wheel is rotated
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            curvol = IPC_GETVOLUME(hwnd_winamp);
            int vol = (curvol / 12);

            if (delta > 0) {
                vol++;
            } else if (delta < 0) {
                vol--;
            }

            // Ensure the volume stays within valid range (0-255)
            vol = std::max(0, std::min(vol, 22));
            vol = std::max(0, std::min(vol * 12, 255));

            // Send the new volume value to Winamp
            SendMessage(hwnd_winamp, WM_WA_IPC, vol, IPC_SETVOLUME);

            // Return zero to indicate that the message has been processed
            return 0;
        }

        default:
            return FALSE;
    }
    return TRUE;
}

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

std::wstring GetInfoText(int TimeMode) {
    // Send the WM_WA_IPC message to get the current output time or track length
    LRESULT waOutputTime;
    LRESULT waTrackLength;

    // Get the current position and track length
    waOutputTime = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
    waTrackLength = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Calculate the time remaining or elapsed based on the TimeMode
    int timeRemaining;
    if (TimeMode == 1) {
        timeRemaining = waTrackLength - waOutputTime;
    } else {
        timeRemaining = waOutputTime;
    }

    // Convert the time to MM:SS format
    std::wstring outputTimeString;
    if (TimeMode == 1){
        outputTimeString = L"-" + formatTime(static_cast<int>(timeRemaining));
    } else {
        outputTimeString = formatTime(static_cast<int>(timeRemaining));
    }

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
            return 128; // Cap the percentage at 128
        } else {
            return percentage;
        }
    } else {
        return 0; // Return 0 if there's no track playing or if track length is not available
    }
}

int setPlayTime(int percentage) {
    // Get the total length of the currently playing track in milliseconds
    LRESULT waOutputTime2 = SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);

    // Convert milliseconds to seconds
    int totalTrackLengthSeconds = (int)(waOutputTime2 / 1000);

    // Calculate the output time in milliseconds from the percentage completion
    int outputTimeSeconds = (int)((double)percentage / 128 * totalTrackLengthSeconds) * 1000;

    return outputTimeSeconds;
}

BOOL IsMenuChecked(HMENU hMenu, UINT uIDCheckItem) {
    return GetMenuState(hMenu, uIDCheckItem, MF_BYCOMMAND) & MF_CHECKED;
}

BOOL IsPLEditVisible() {
    return IsMenuChecked(WinampMenu, WINAMP_OPTIONS_PLEDIT);
}

BOOL IsEQVisible() {
    return IsMenuChecked(WinampMenu, WINAMP_OPTIONS_EQ);
}

void DrawMainBox(HWND hMainBox, int res) {
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
            //HBRUSH hBrushBg2 = CreateSolidBrush(RGB(0, 0, 0)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            int fontSize = 34; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            textRect = { 10, 1, width - 7, height - 10 }; // Adjust the coordinates and size as needed
            const std::wstring infoText = GetInfoText(TimeMode);
            std::wstring timeText;

            ULONGLONG currentTime = GetTickCount64();
            DWORD elapsedTime = static_cast<DWORD>(currentTime - pauseStartTime);

            // Set timeText to flashing SongTimer if Winamp is paused
            if (res == 3 && elapsedTime % SECOND_DURATION < SECOND_DURATION / 2) {
                timeText = L"  :  "; // Flashing SongTimer
            } else {
                timeText = (res == 0) ? L"  :  " : infoText; // Default to infoText or SongTimer
            }

            DrawTextW(hdcBuffer, timeText.c_str(), timeText.size(), &textRect, DT_RIGHT | DT_TOP);
            // Restore the original font
            SelectObject(hdcBuffer, hOldFont);
            DeleteObject(hFont);

            // Get SA data
            sadata = export_sa_get();

/*          // Draw the rectangle around the visualizer area
            RECT visRect = {12 * 2, 18 * 2, (12 + 77) * 2, (19 + 18) * 2}; // x, y, width, height
            FillRect(hdcBuffer, &visRect, hBrushBg2); */

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
            HBITMAP hResBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PLAYPAUS));

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
            if (VisMode == 2) {
                if (sadata) {
                    // Get the array of colors for visualization mode 2
                    COLORREF* oscColors = osccolors(colors);

                    for (int x = 0; x < 75; x++) {
                        signed char y = sadata[x + 75];
                        int intValue = y + 7;
                        intValue = intValue < 0 ? 0 : (intValue > 16 - 1 ? 16 - 1 : intValue);

                        // Calculate the endpoint of the line based on intValue
                        int endpointY = (intValue * 2) + 40; // Adjusted for starting at y=42

                        // Select the color from oscColors based on intValue
                        COLORREF lineColor = oscColors[intValue];

                        // Set the line color
                        HPEN hPen = CreatePen(PS_SOLID, 1, lineColor);
                        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);

                        // Grab the first point of sadata so that we don't have a weird
                        // line that's just static
                        if (x == 0) {
                            MoveToEx(hdcBuffer, 26, 40 + (intValue * 2), NULL);
                        } else {
                            LineTo(hdcBuffer, (x * 2) + 26, endpointY);
                        }

                        // Restore the original pen
                        SelectObject(hdcBuffer, hOldPen);
                        DeleteObject(hPen);
                    }

                    // Release memory allocated for oscColors array
                    releaseColorRefs(oscColors);
                }
            }

            else if (VisMode == 1) {
                if (sadata) {
                    static float sapeaks[150];
                    static char safalloff[150];
                    static char sapeaksdec[150];
                    signed char sadata2[150];

                    for (int x = 0; x < 75; x++) {
                        sadata2[x] = sadata[x];
                        signed char y = safalloff[x];
                        signed char y2 = sapeaks[x];

                        safalloff[x] = safalloff[x] - 2;

                        // okay this is really funny
                        // somehow the internal vis data for winamp/wacup can just, wrap around
                        // but here it didnt, until i saw my rect drawing *under* its intended area
                        // and i just figured out that winamp's vis box just wraps that around
                        // this is really funny to me
                        if (sadata2[x] < 0) {
                            sadata2[x] = sadata2[x] + 127;
                        }
                        if (sadata2[x] >= 15) {
                            sadata2[x] = 15;
                        }

                        if (safalloff[x] <= sadata2[x]) {
                            safalloff[x] = sadata2[x];
                        }

                        sapeaks[x] = sapeaks[x] - 1.0f / 5.0f;
                        if (safalloff[x] > sapeaks[x]) {
                            sapeaks[x] = safalloff[x];
                        }

                        int intValue = -y + 17;
                        int intValue2 = -y2 + 16;

                        for (int y3 = 0; y3 < 16; y3++) {
                            const RECT analyzer = {26 + (x * 2), 70, 26 + (x * 2) + 2, (intValue * 2) + 36};
                            COLORREF analyzerColor = RGB(colors[intValue].r, colors[intValue].g, colors[intValue].b);
                            HBRUSH hBrushAnalyzer = CreateSolidBrush(analyzerColor);
                            FillRect(hdcBuffer, &analyzer, hBrushAnalyzer);
                            DeleteObject(hBrushAnalyzer);
                        }

                        // Draw peaks using a single color
                        if (intValue2 >= 16) {
                            continue;
                        }
                        const RECT peaks = {26 + (x * 2), 37 + (intValue2 * 2), 26 + (x * 2) + 2, (intValue2 * 2) + 39};
                        COLORREF peaksColor = RGB(colors[23].r, colors[23].g, colors[23].b);
                        HBRUSH hBrushPeaks = CreateSolidBrush(peaksColor);
                        FillRect(hdcBuffer, &peaks, hBrushPeaks);
                        DeleteObject(hBrushPeaks);
                    }
                }
            } else if (VisMode == 3) {
                // SORRY NOTHING
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
                        //FillRect(hdcBuffer, &visRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
            // Copy the off-screen buffer to the screen
            BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);

            // Clean up
            SelectObject(hdcBuffer, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcBuffer);
            DeleteObject(hBrushBg);

            EndPaint(hMainBox, &ps);
}

void DrawBitrate(HWND hBitrate, int res, int bitr) {
    PAINTSTRUCT ps;
    RECT rc;
    GetClientRect(hBitrate, &rc);
            HDC hdc = BeginPaint(hBitrate, &ps);
            GetClientRect(hBitrate, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            HDC hdcBuffer = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            int fontSize = 13; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            RECT textRect = { 0, 3, width - 5, height }; // Adjust the coordinates and size as needed
            std::string bitr_str = std::to_string(bitr);
            std::string bitrate_str = "";
            if (res == 0) {
                bitrate_str = "";
            } else {
                bitrate_str = bitr_str;
            }
            DrawTextA(hdcBuffer, bitrate_str.c_str(), -1, &textRect, DT_RIGHT | DT_TOP);

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
}

void DrawSamplerate(HWND hSamplerate, int res, int smpr) {
    PAINTSTRUCT ps;
    RECT rc;
    GetClientRect(hSamplerate, &rc);
            HDC hdc = BeginPaint(hSamplerate, &ps);
            GetClientRect(hSamplerate, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            HDC hdcBuffer = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            int fontSize = 13; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            RECT textRect = { 0, 3, width - 5, height }; // Adjust the coordinates and size as needed
            std::string smpr_str = std::to_string(smpr);
            std::string samplerate_str = "";
            if (res == 0) {
                samplerate_str = "";
            } else {
                samplerate_str = smpr_str;
            }
            DrawTextA(hdcBuffer, samplerate_str.c_str(), -1, &textRect, DT_RIGHT | DT_TOP);

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
}

void DrawSongTicker(HWND hSongTicker) {
    PAINTSTRUCT ps;
    RECT rc;
    GetClientRect(hSongTicker, &rc);
            HDC hdc = BeginPaint(hSongTicker, &ps);
            GetClientRect(hSongTicker, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            // Create a memory device context for double buffering
            HDC hdcBuffer = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

            HBRUSH hBrushBg = CreateSolidBrush(RGB(255, 255, 255)); // White color
            FillRect(hdcBuffer, &rc, hBrushBg);

            // Set the font size
            int fontSize = 13; // Change this to the desired font size
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
            HFONT hOldFont = (HFONT)SelectObject(hdcBuffer, hFont);

            // Draw text
            RECT textRect = { 6, 3, width - 6, height }; // Adjust the coordinates and size as needed
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
}

// Plugin getter function
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}