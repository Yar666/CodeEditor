#include "WINDOWAREA.h"
LRESULT CALLBACK WindowAreaProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ATOM WindowAreaRegisterClass(HINSTANCE _hInstance, COLORREF color) {
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowAreaProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(color);
    wcex.lpszClassName = L"WINDOWAREA";
    return RegisterClassExW(&wcex);
}
LRESULT CALLBACK WindowAreaProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hParent = GetParent(hWnd);
    switch (message) {
        case WM_COMMAND: {
            if (hParent) {
                SendMessage(hParent, WM_COMMAND, wParam, lParam);
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
