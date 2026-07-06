// CodeEditorTest.cpp : Defines the entry point for the application.
//

#include "CodeEditorMenu.h"
#include "framework.h"
#include <unordered_map>
#include "CodeEditorTest.h"
#include "CustomTextBox.h"

#include "FileExplorer.h"
#include "FileManager.h"
#include <iostream>
#include <shlobj.h>
#define MAX_LOADSTRING 100
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
using namespace std;
static unordered_map<wstring, HWND> ChildControlWindows;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    AllocConsole();
    FILE* stream;
    if (freopen_s(&stream, "CONOUT$", "w", stdout) != 0) {
        std::cerr << "Ошибка: не удалось перенаправить stdout в консоль" << std::endl;
        return 1;
    }
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CODEEDITORTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CODEEDITORTEST));

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

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CODEEDITORTEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CODEEDITORTEST);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; 

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED|WS_CAPTION | WS_SYSMENU| WS_MINIMIZEBOX,
      700, 300, 1000, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   MyTextBoxRegisterClass(hInst);
   CodeEditorMenuRegisterClass(hInst);
   FileExplorerRegisterClass(hInst);
   ChildControlWindows[L"Menu"] = CreateCodeEditorMenu(hWnd, 0, 0, 1000, 50, hInst);
   ChildControlWindows[L"CodeEditor"] = CreateCustomTextBox(hWnd, 300, 50, 700, 690, hInst);
   ChildControlWindows[L"FileExplorer"] = CreateFileExplorer(hWnd, 0, 50, 300, 690, hInst);
 

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE: {
        break;
    }
    case WM_LBUTTONDOWN: {


        break;
    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case ID_FILE_OPENFOLDER: {
                FileManager& manager{ FileManager::getInstance() };
                //if (manager.getCurrentDirectory().empty()) {
                //    manager.setDirectory(L"H:\\C++_proj\\winapi\\CodeEditorTest\\CodeEditorTest\\TestDir\\");
                //    break;
                //}
                IFileDialog* pfd{};
                HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
                if (SUCCEEDED(hr)) {
                    DWORD dwOptions;
                    pfd->GetOptions(&dwOptions);
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);

                    hr = pfd->Show(NULL);
                    if (SUCCEEDED(hr)) {
                        IShellItem* psi;
                        hr = pfd->GetResult(&psi);
                        if (SUCCEEDED(hr)) {
                            PWSTR pszPath;
                            psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                            if (pszPath) {
                                wcout << L"Selected folder: " << pszPath << std::endl;
                                FileManager::getInstance().setDirectory(pszPath);
                            }
                            else {
                                wcout << L"Selectint folder error" << std::endl;
                            }
                            CoTaskMemFree(pszPath);
                            psi->Release();
                        }
                    }
                    pfd->Release();
                }
                break;
            }
            case IDM_ABOUT:
                //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                FreeConsole();
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
