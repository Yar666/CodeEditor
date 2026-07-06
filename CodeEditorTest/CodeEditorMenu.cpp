#include "CodeEditorMenu.h"
#include "WINDOWAREA.h"
#include "FileManager.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
using namespace std;
#define RunButton 700
#define SaveButton 701
#define CloseButton 702
unordered_map<wstring, HWND> ChildMenuControlWindows;
HINSTANCE hInstMenu;
ATOM CodeEditorMenuRegisterClass(HINSTANCE hInstance)
{
    hInstMenu = hInstance;
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = CodeEditorMenuProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(30, 30, 30));
    wcex.lpszClassName = L"CodeEditorMenu";
    return RegisterClassExW(&wcex);
}
HWND CreateCodeEditorMenu(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst) {
    return CreateWindow(L"CodeEditorMenu", L"", WS_CHILD | WS_VISIBLE, x, y, width, height, hwnd, NULL, hInst, NULL);
}
void reDrawButtonsInArea(HWND hWnd, size_t index) {
    unordered_map<size_t, FileManager::_File>& FileButtons = FileManager::getInstance().getOpenedFiles();
    RECT rt;
    GetClientRect(hWnd, &rt);
    HDC hdc = GetDC(hWnd);
    RECT textRect = { 0, 0, 0, 0 };
    SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
    GetScrollInfo(ChildMenuControlWindows[L"FileScrollArea"], SB_HORZ, &si);
    int x{-si.nPos};
    for (auto& el : FileButtons) {
        DrawText(hdc, el.second.name.c_str(), (int)el.second.name.length(), &textRect, DT_CALCRECT);
        textRect.right += 10;
        if (FileButtons[el.first].hWndButton != NULL) { //Moving
            SetWindowPos(FileButtons[el.first].hWndButton, NULL, x, 0, textRect.right, rt.bottom, SWP_NOSIZE | SWP_NOZORDER);
        }
        else {
            FileButtons[el.first].hWndButton = CreateWindowEx( //Drawing
                0, L"BUTTON", el.second.name.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x, 0, textRect.right, rt.bottom, ChildMenuControlWindows[L"FileScrollArea"], (HMENU)el.second.hmenu, GetModuleHandle(NULL), NULL
            );
        }

        x += textRect.right + 5;
    }
    if (x+si.nPos > rt.right) {
        si.nMax = x + si.nPos - rt.right+5;
    }
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
    ShowScrollBar(hWnd, SB_BOTH, FALSE);

    ReleaseDC(hWnd, hdc);
}
void onCodeEditorSelectFile(HWND hWnd, size_t index) {
    InvalidateRect(hWnd, NULL, TRUE);
}
LRESULT CALLBACK CodeEditorMenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    unordered_map<size_t, FileManager::_File>& FileButtons = FileManager::getInstance().getOpenedFiles();
    FileManager& manager = FileManager::getInstance();
    RECT rt;
    GetClientRect(hWnd, &rt);
    RECT currentFileTextBox{ 120,0, rt.right, rt.bottom / 2 };
    switch (message)
    {
    case WM_CREATE: {
        

        ChildMenuControlWindows[L"RunButton"] = CreateWindowEx(
            0, L"BUTTON", L"RUN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON,
            0, 0, 50, 50, hWnd, (HMENU)RunButton, hInstMenu, NULL
        );
        HICON hIcon = LoadIcon(hInstMenu, MAKEINTRESOURCE(IDI_ICON1));
        SendMessage(ChildMenuControlWindows[L"RunButton"], BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

        ChildMenuControlWindows[L"SaveButton"] = CreateWindowEx(
            0, L"BUTTON", L"SAVE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON,
            60, 0, 50, 50, hWnd, (HMENU)SaveButton, hInstMenu, NULL
        );
        hIcon = LoadIcon(hInstMenu, MAKEINTRESOURCE(IDI_ICON2));
        SendMessage(ChildMenuControlWindows[L"SaveButton"], BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);
        ChildMenuControlWindows[L"CloseButton"] = CreateWindowEx(
            0, L"BUTTON", L"Close file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rt.right - 90, 0, 70, rt.bottom / 2, hWnd, (HMENU)CloseButton, hInstMenu, NULL
        );
        WindowAreaRegisterClass(hInstMenu, RGB(32, 38, 52));
        ChildMenuControlWindows[L"FileScrollArea"] = CreateWindowEx(
            0, L"WINDOWAREA", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL,
            120, rt.bottom / 2, rt.right-120, rt.bottom / 2, hWnd, NULL, hInstMenu, NULL
        );
        SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
        GetScrollInfo(ChildMenuControlWindows[L"FileScrollArea"], SB_HORZ, &si);
        si.nMax = 0;
        SetScrollInfo(ChildMenuControlWindows[L"FileScrollArea"], SB_HORZ, &si, FALSE);
        ShowScrollBar(ChildMenuControlWindows[L"FileScrollArea"], SB_BOTH, FALSE);
        manager.registerOnOpenFile(reDrawButtonsInArea, ChildMenuControlWindows[L"FileScrollArea"]);
        manager.registerOnSelectFile(onCodeEditorSelectFile, hWnd);
        manager.registerOnCloseFile(reDrawButtonsInArea, ChildMenuControlWindows[L"FileScrollArea"]);
        break;
    }
    case WM_MOUSEWHEEL: {
        RECT rt, textRect{};
        GetClientRect(hWnd, &rt);
        int delta{ GET_WHEEL_DELTA_WPARAM(wParam) };
        cout << "WM_MOUSEWHEEL: delta " << delta << endl;
        SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
        GetScrollInfo(ChildMenuControlWindows[L"FileScrollArea"], SB_HORZ, &si);
        int newPos = si.nPos;
        if (delta < 0) {
            newPos += 10 * abs(delta / 120);
        }
        else {
            newPos -= 10 * abs(delta / 120);
        }
        newPos = max(si.nMin, min(si.nMax, newPos));
        si.nPos = newPos;
        SetScrollInfo(ChildMenuControlWindows[L"FileScrollArea"], SB_HORZ, &si, FALSE);
        ShowScrollBar(ChildMenuControlWindows[L"FileScrollArea"], SB_BOTH, FALSE);
        int x{};
        HDC hdc = GetDC(hWnd);
        for (auto& el : FileButtons) {
            DrawText(hdc, el.second.name.c_str(), (int)el.second.name.length(), &textRect, DT_CALCRECT);
            textRect.right += 10;
            SetWindowPos(FileButtons[el.first].hWndButton, NULL, x - newPos, 0, textRect.right, rt.bottom / 2, SWP_NOSIZE | SWP_NOZORDER);
            x += textRect.right + 5;
        }
        
        ReleaseDC(hWnd, hdc);
        break;
    }
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED) {
            SetFocus(hWnd);
        }
        int wmId = LOWORD(wParam);

        if (FileButtons.count(wmId)) {
            manager.setActiveFile(FileButtons[wmId].hmenu);
            InvalidateRect(hWnd, &currentFileTextBox, TRUE);
        }
        switch (wmId)
        {
        case RunButton: {
            cout << "RunButton" << endl;
            manager.run();
            break;
        }
        case SaveButton: {
            cout << "SaveButton" << endl;
            manager.saveFile();
            break;
        }
        case CloseButton: {
            cout << "CloseButton" << endl;
            manager.closeFile(manager.getActiveFile());
            InvalidateRect(hWnd, &currentFileTextBox, TRUE);
            break;
        }
        default:
            cout << wmId << endl;
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        RECT rt;
        GetClientRect(hWnd, &rt);
        HDC hdc = BeginPaint(hWnd, &ps);
        wstring text{ L"Current file: " };
        size_t activeFile = manager.getActiveFile();
        if (activeFile == wstring::npos) {
            text += L"NULL";
        }
        else {
            text += FileButtons[activeFile].name;
        }
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, text.c_str(), text.size(), &currentFileTextBox, NULL);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_MOUSEACTIVATE: {
        cout << "Menu WM_MOUSEACTIVATE" << endl;
        return MA_ACTIVATE;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}