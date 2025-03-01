#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// // Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>
// Windows Header Files
#include <windows.h>

#include "resource.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <charconv>
#include <format>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_LOADSTRING 100

// Global Variables:
WCHAR titleBarText[MAX_LOADSTRING];
WCHAR windowClassName[MAX_LOADSTRING];

enum FocusFunction : uint32_t
{
    None,
    SetActiveWindow_,
    SetForegroundWindow_,
    SetFocus_,
    SwitchToThisWindow_,
    BringWindowToTop_,
    SetWindowPosition_,
};

FocusFunction pendingFocusFunction = FocusFunction::None;
wchar_t const* pendingFocusFunctionName = L"";
int32_t timerCountDownRemaining = 0;
int32_t timerCountDownDefault = 5 * 1000;
constexpr uint32_t timerCountDownId = 1234;

// Forward declarations of functions included in this code module:
ATOM RegisterCustomClass(HINSTANCE instanceHandle);
BOOL InitializeInstance(HINSTANCE instanceHandle, int showCommand);
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int APIENTRY wWinMain(
    _In_ HINSTANCE instanceHandle,
    _In_opt_ HINSTANCE previousInstanceHandle,
    _In_ LPWSTR commandLine,
    _In_ int showCommand
)
{
    UNREFERENCED_PARAMETER(previousInstanceHandle);
    UNREFERENCED_PARAMETER(commandLine);

    // Initialize global strings
    LoadStringW(instanceHandle, IDS_APP_TITLE, titleBarText, MAX_LOADSTRING);
    LoadStringW(instanceHandle, IDC_SETFOREGROUNDACTIVEFOCUSWINDOW, windowClassName, MAX_LOADSTRING);
    RegisterCustomClass(instanceHandle);

    // Perform application initialization:
    if (!InitializeInstance(instanceHandle, showCommand))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(instanceHandle, MAKEINTRESOURCE(IDC_SETFOREGROUNDACTIVEFOCUSWINDOW));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return int(msg.wParam);
}

ATOM RegisterCustomClass(HINSTANCE instanceHandle)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WindowProcedure;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = instanceHandle;
    wcex.hIcon          = LoadIcon(instanceHandle, MAKEINTRESOURCE(IDI_SETFOREGROUNDACTIVEFOCUSWINDOW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = windowClassName;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitializeInstance(HINSTANCE instanceHandle, int showCommand)
{
   HWND hwnd = CreateWindowW(
       windowClassName,
       titleBarText,
       WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       600, // CW_USEDEFAULT,
       400, // CW_USEDEFAULT,
       nullptr,
       nullptr,
       instanceHandle,
       nullptr
   );

   if (!hwnd)
   {
      return FALSE;
   }

   ShowWindow(hwnd, showCommand);
   UpdateWindow(hwnd);

   return TRUE;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            auto setTimer = [=](FocusFunction func, wchar_t const* functionName)
            {
                pendingFocusFunction = func;
                pendingFocusFunctionName = functionName;
                timerCountDownRemaining = timerCountDownDefault;
                SetTimer(hwnd, timerCountDownId, 1000, 0);
                PostMessage(hwnd, WM_TIMER, timerCountDownId, LPARAM(0));
            };

            switch (wmId)
            {
            case IDM_EXIT: DestroyWindow(hwnd); break;
            case IDM_SetActiveWindow: setTimer(SetActiveWindow_, L"SetActiveWindow"); break;
            case IDM_SetForegroundWindow: setTimer(SetForegroundWindow_, L"SetForegroundWindow"); break;
            case IDM_SetFocus: setTimer(SetFocus_, L"SetFocus"); break;
            case IDM_SwitchToThisWindow: setTimer(SwitchToThisWindow_, L"SwitchToThisWindow"); break;
            case IDM_BringWindowToTop: setTimer(BringWindowToTop_, L"BringWindowToTop"); break;
            case IDM_SetWindowPosition: setTimer(SetWindowPosition_, L"SetWindowPosition"); break;
            default: return DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect = {};
            GetClientRect(hwnd, /*out*/ &rect);

            constexpr std::wstring_view message =
                L"Press the following keys:\r\n"
                L"• A - SetActiveWindow\r\n"
                L"• F - SetForegroundWindow\r\n"
                L"• K - SetFocus\r\n"
                L"• S - SwitchToThisWindow\r\n"
                L"• B - BringWindowToTop\r\n"
                L"• P - SetWindowPosition\r\n";

            // Draw label.
            HFONT font = HFONT(GetStockObject(DEFAULT_GUI_FONT));
            HGDIOBJ previousFont = SelectObject(hdc, font);
            DrawText(hdc, message.data(), int(message.size()), &ps.rcPaint, DT_NOPREFIX);
            SelectObject(hdc, previousFont);

            EndPaint(hwnd, &ps);
        }
        break;

    case WM_TIMER:
        switch (wParam)
        {
        case timerCountDownId:
            if (timerCountDownRemaining > 0)
            {
                auto s = std::format(L"{} seconds until calling {}", timerCountDownRemaining, pendingFocusFunctionName);
                SetWindowText(hwnd, s.c_str());
                timerCountDownRemaining -= 1000; // 1 second tick.
            }
            else
            {
                KillTimer(hwnd, wParam);
                switch (pendingFocusFunction)
                {
                case FocusFunction::SetActiveWindow_: SetActiveWindow(hwnd); break;
                case FocusFunction::SetForegroundWindow_: SetForegroundWindow(hwnd); break;
                case FocusFunction::SetFocus_: SetFocus(hwnd); break;
                case FocusFunction::SwitchToThisWindow_: SwitchToThisWindow(hwnd, true); break;
                case FocusFunction::BringWindowToTop_: BringWindowToTop(hwnd); break;
                case FocusFunction::SetWindowPosition_:
                    SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
                    break;
                }
                auto s = std::format(L"Executed focus API call {}", pendingFocusFunctionName);
                SetWindowText(hwnd, s.c_str());
            }
            return 0;
        default:
            return 1;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
