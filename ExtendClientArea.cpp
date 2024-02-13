//
// ----- BUILD COMMAND
// $ g++ -shared -o ExtendClientArea.dll ExtendClientArea.cpp -lgdi32 -ldwmapi -municode -lcomctl32
//
// ----- HOW TO USE
// --- 1. Compile the DLL using the build command
// self.dll_path = os.path.join(self.baseDir, 'SmPlugins', 'ExtendClientArea.dll')
// self.lib = CDLL(self.dll_path)
// self.hwnd = int(self.winId())
// self.lib.InitializeCustomWindowProc(self.hwnd)
// self.lib.ExtendClientArea(self.hwnd)
//
// --- 2. Do something with the window like:
// def closeNative(self):
//     self.lib.CloseCommand(self.hwnd)
// def maximizeNative(self):
//     self.lib.MaximizeCommand(self.hwnd)
// def minimizeNative(self):
//     self.lib.MinimizeCommand(self.hwnd)

#include <windows.h>
#include <dwmapi.h>
#include <Windowsx.h>
#include <iostream>

// Global variable to store the original window procedure
WNDPROC original_proc;

// Lock variables
BOOL maxButtonLocked = FALSE;

// Modifier function to change the window procedure
LRESULT CALLBACK CustomWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_NCCALCSIZE:
            if (wParam == TRUE) {
                NCCALCSIZE_PARAMS* pNCCSP = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

                if (!IsZoomed(hwnd)) {
                    // For non-maximized state
                    pNCCSP->rgrc[0].top += 1;
                    pNCCSP->rgrc[0].left += GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    pNCCSP->rgrc[0].right -= GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    pNCCSP->rgrc[0].bottom -= 1;
                } else {
                    // For maximized state
                    pNCCSP->rgrc[0].top += 8;
                    pNCCSP->rgrc[0].left += GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    pNCCSP->rgrc[0].right -= GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    pNCCSP->rgrc[0].bottom -= GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                }

                return 0; // Return 0 to indicate that the client area has been modified
            }
            break;

        case WM_NCHITTEST:
        {
            // Default Window Procedure for messages
            LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);

            if(hit == HTCLIENT) {
                POINT pt;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(hwnd, &pt);

                RECT rcWindow;
                GetWindowRect(hwnd, &rcWindow);  // Size of the resize border
                int border_width = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int border_height = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int top_padding = 8;
                MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcWindow, 2);
                    
                // Determine positions
                if(pt.x < rcWindow.left + border_width)
                    hit = HTLEFT; // Left border
                else if(pt.x > rcWindow.right - border_width)
                    hit = HTRIGHT; // Right border

                if(pt.y < rcWindow.top + border_height)
                    hit = hit == HTLEFT ? HTTOPLEFT : hit == HTRIGHT ? HTTOPRIGHT : HTTOP; // Top border
                else if(pt.y > rcWindow.bottom - border_height)
                    hit = hit == HTLEFT ? HTBOTTOMLEFT : hit == HTRIGHT ? HTBOTTOMRIGHT : HTBOTTOM; // Bottom border

                RECT rectMaxButton = {
                    rcWindow.right - rcWindow.left - GetSystemMetrics(SM_CXSIZEFRAME) - 3 * GetSystemMetrics(SM_CXSIZE), // X-Position
                    GetSystemMetrics(SM_CXSIZEFRAME), // Y-Position
                    rcWindow.right - rcWindow.left - GetSystemMetrics(SM_CXSIZEFRAME) - 2 * GetSystemMetrics(SM_CXSIZE), // X-Position + Width
                    GetSystemMetrics(SM_CYCAPTION) + top_padding // Y-Position + Height + Padding
                };

                // if (PtInRect(&rectMaxButton, pt))
                //     printf("HIT!!! in rect: %d, %d, %d, %d, by POINT: %d, %d, is locked? -> %d\n", rectMaxButton.left, rectMaxButton.top, rectMaxButton.right, rectMaxButton.bottom, pt.x, pt.y, maxButtonLocked);
                // else 
                //     printf("No hit in rect: %d, %d, %d, %d, by POINT: %d, %d, is locked? -> %d\n", rectMaxButton.left, rectMaxButton.top, rectMaxButton.right, rectMaxButton.bottom, pt.x, pt.y, maxButtonLocked);
            
               
                if (PtInRect(&rectMaxButton, pt) && !maxButtonLocked)
                    hit = HTMAXBUTTON; // Maximize button
                else
                    maxButtonLocked = TRUE;

                return hit;
            }
            return hit;
        }

        default:
            return CallWindowProc(original_proc, hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE; // DLL successfully initialized
}

// Set the custom window procedure and store the original one
extern "C" __declspec(dllexport) void InitializeCustomWindowProc(HWND hwnd) {
    original_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWindowProc);
}

// Restore the original window procedure
extern "C" __declspec(dllexport) void RestoreOriginalWindowProc(HWND hwnd) {
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)original_proc);
}

extern "C" __declspec(dllexport) void ExtendClientArea(HWND hwnd) {
    LONG_PTR lStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    lStyle |= (WS_CAPTION | WS_THICKFRAME);

    SetWindowLongPtr(hwnd, GWL_STYLE, lStyle);

    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
}

// Utility functions to change the window style
extern "C" __declspec(dllexport) void SetMaxButtonLock(HWND hwnd, BOOL lock) {
    maxButtonLocked = lock;
    // InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

}

extern "C" __declspec(dllexport) void RestoreCaptionCommand(HWND hwnd) {
    LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, lStyle | WS_CAPTION);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

extern "C" __declspec(dllexport) void RemoveCaptionCommand(HWND hwnd) {
    LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, lStyle & ~WS_CAPTION);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

extern "C" __declspec(dllexport) void SetWindowPosCommand(HWND hwnd, int x, int y, int cx, int cy, UINT uFlags) {
    SetWindowPos(hwnd, NULL, x, y, cx, cy, uFlags);
}

extern "C" __declspec(dllexport) void MoveWindowCommand(HWND hwnd, int x, int y) {
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

extern "C" __declspec(dllexport) void ReleaseCaptureCommand(HWND hwnd) {
    ReleaseCapture();
    SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

extern "C" __declspec(dllexport) void SetCaptureCommand(HWND hwnd) {
    SetCapture(hwnd);
}

extern "C" __declspec(dllexport) void SendMessageWCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SendMessageW(hwnd, msg, wParam, lParam);
}

extern "C" __declspec(dllexport) void ExecuteSystemCommand(HWND hwnd, UINT cmd) {
    SendMessage(hwnd, WM_SYSCOMMAND, cmd, 0);
}

extern "C" __declspec(dllexport) void CloseCommand(HWND hwnd) {
    ExecuteSystemCommand(hwnd, SC_CLOSE);
}

extern "C" __declspec(dllexport) void MinimizeCommand(HWND hwnd) {
    ExecuteSystemCommand(hwnd, SC_MINIMIZE);
}

extern "C" __declspec(dllexport) void MaximizeCommand(HWND hwnd) {
    if (IsZoomed(hwnd))
      ExecuteSystemCommand(hwnd, SC_RESTORE);
    else
      ExecuteSystemCommand(hwnd, SC_MAXIMIZE);
}

extern "C" __declspec(dllexport) BOOL IsWindowMaximized(HWND hwnd) {
    return IsZoomed(hwnd);
}

extern "C" __declspec(dllexport) BOOL IsWindowMinimized(HWND hwnd) {
    return IsIconic(hwnd);
}