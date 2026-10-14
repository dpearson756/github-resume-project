// Clock.cpp : Defines the entry point for the application.
//

#define NOMINMAX

#include "framework.h"
#include "Clock.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

#define MAX_LOADSTRING 100

// Global Variables
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
bool g_fullscreen = false;                                  // only one definition
WINDOWPLACEMENT g_prevPlacement = { sizeof(g_prevPlacement) };

// Function Prototypes
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                ToggleFullscreen(HWND hWnd);            
void DrawClock(HDC hdc, const RECT& rc);

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLOCK, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLOCK));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);

    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 3);

    // Remove File/About menu
    wcex.lpszMenuName = nullptr;

    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, SW_MAXIMIZE);
    UpdateWindow(hWnd);

    return TRUE;
}

void DrawClock(HDC hdc, const RECT& rc)
{
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Background
    HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    SYSTEMTIME st;
    GetLocalTime(&st);

    // Text strings
    int hour12 = st.wHour % 12;
    if (hour12 == 0) hour12 = 12;

    wchar_t timeBuf[32];
    swprintf_s(timeBuf, L"%d:%02d:%02d %s", hour12, st.wMinute, st.wSecond,
        st.wHour < 12 ? L"AM" : L"PM");

    wchar_t dayBuf[64];
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, L"dddd", dayBuf, 64, nullptr);

    wchar_t dateBuf[64];
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, L"MMMM d, yyyy", dateBuf, 64, nullptr);

    // Helpers
    auto makeFont = [](int height, int weight)
        {
            return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        };

    auto measure = [&](HFONT f, const wchar_t* text)
        {
            SIZE sz = {};
            HGDIOBJ old = SelectObject(hdc, f);
            GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &sz);
            SelectObject(hdc, old);
            return sz;
        };

    // Font sizes: time is biggest, scaled to fit the width
    int timeH = h / 4;
    HFONT timeFont = makeFont(timeH, FW_LIGHT);
    SIZE timeSz = measure(timeFont, timeBuf);
    if (timeSz.cx > w * 0.9)
    {
        DeleteObject(timeFont);
        timeH = (int)(timeH * (w * 0.9) / timeSz.cx);
        timeFont = makeFont(timeH, FW_LIGHT);
        timeSz = measure(timeFont, timeBuf);
    }

    int dayH = (int)(timeH * 0.55);
    int dateH = (int)(timeH * 0.40);
    HFONT dayFont = makeFont(dayH, FW_SEMIBOLD);
    HFONT dateFont = makeFont(dateH, FW_NORMAL);

    // Shrink the date if it's too wide for narrow windows
    SIZE dateSz = measure(dateFont, dateBuf);
    if (dateSz.cx > w * 0.9)
    {
        DeleteObject(dateFont);
        dateH = (int)(dateH * (w * 0.9) / dateSz.cx);
        dateFont = makeFont(dateH, FW_NORMAL);
        dateSz = measure(dateFont, dateBuf);
    }
    SIZE daySz = measure(dayFont, dayBuf);

    // Stack the three lines and center the block vertically
    int gap = h / 50;
    int totalH = daySz.cy + gap + timeSz.cy + gap + dateSz.cy;
    int y = rc.top + (h - totalH) / 2;

    SetBkMode(hdc, TRANSPARENT);

    auto drawLine = [&](HFONT f, const wchar_t* text, int height, COLORREF color)
        {
            HGDIOBJ old = SelectObject(hdc, f);
            SetTextColor(hdc, color);
            RECT r = { rc.left, y, rc.right, y + height };
            DrawTextW(hdc, text, -1, &r, DT_CENTER | DT_TOP | DT_SINGLELINE);
            SelectObject(hdc, old);
            y += height + gap;
        };

    drawLine(dayFont, dayBuf, daySz.cy, RGB(255, 0, 0));    // day title
    drawLine(timeFont, timeBuf, timeSz.cy, RGB(255, 0, 0));   // time
    drawLine(dateFont, dateBuf, dateSz.cy, RGB(160, 0, 0));   // date

    DeleteObject(timeFont);
    DeleteObject(dayFont);
    DeleteObject(dateFont);
}


LRESULT CALLBACK WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(hWnd, 1, 1000, NULL);
        return 0;

    case WM_TIMER:
        InvalidateRect(hWnd, NULL, FALSE);   // FALSE: we repaint everything ourselves
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
        if (wParam == VK_F11 && !(lParam & (1 << 30)))   // ignore auto-repeat
        {
            ToggleFullscreen(hWnd);
        }
        else if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HGDIOBJ oldBmp = SelectObject(memDC, bmp);

        DrawClock(memDC, rc);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(bmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

void ToggleFullscreen(HWND hWnd)
{
    DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);

    if (!g_fullscreen)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hWnd, &g_prevPlacement) &&
            GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            SetWindowLongPtr(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hWnd, HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            g_fullscreen = true;
        }
    }
    else
    {
        SetWindowLongPtr(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &g_prevPlacement);
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen = false;
    }
}
