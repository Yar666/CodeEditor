#include "FileExplorer.h"
#include "FileManager.h"
#include <commctrl.h>
#include <windowsx.h>
#include <fstream>
#include <iostream>
#include <shellapi.h>
HINSTANCE hInstExplorer;
HWND hTreeView;
unordered_map<HTREEITEM, wstring> treeItemPaths;
ATOM FileExplorerRegisterClass(HINSTANCE hInstance)
{
    hInstExplorer = hInstance;
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = FileExplorerProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(38, 40, 49));
    wcex.lpszClassName = L"FileExplorer";
    return RegisterClassExW(&wcex);
}
HWND CreateFileExplorer(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst) {
    return CreateWindow(L"FileExplorer", L"", WS_CHILD | WS_VISIBLE, x, y, width, height, hwnd, NULL, hInst, NULL);
}
//enum class FILETYPE {
//    DIR,
//    FILE,
//};
//struct FileBrowser {
//    wstring fileName;
//    wstring path;
//    FILETYPE type;
//    bool isDirOpen{ false };
//    bool isShown{ true };
//    HWND buttonHWND;
//    size_t buttonMenu;
//    unordered_set<size_t> Childrens;
//    const FileBrowser* parent{nullptr};
//    int spacing{ 0 };
//    int y{ 0 };
//    int buttonWidth;
//};
//enum class RollupType {
//    COLLAPSE,
//    EXPAND,
//};
//void moveFoldersGroup(unordered_map<size_t, FileBrowser>& FilesList, FileBrowser& parent, int& startY) {
//    parent.y = startY;
//    SetWindowPos(parent.buttonHWND, NULL, parent.spacing, parent.y, parent.buttonWidth, buttonHeight, SWP_NOSIZE | SWP_NOZORDER);
//    startY += buttonHeight + 2;
//    for (auto& el : parent.Childrens) {
//        if (!FilesList[el].isShown) {
//            continue;
//        }
//        if (FilesList[el].type == FILETYPE::DIR && FilesList[el].isDirOpen && !FilesList[el].Childrens.empty()) {
//            moveFoldersGroup(FilesList, FilesList[el], startY);
//        }
//        else{
//            FilesList[el].y = startY;
//            SetWindowPos(FilesList[el].buttonHWND, NULL, FilesList[el].spacing, FilesList[el].y, FilesList[el].buttonWidth, buttonHeight, SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
//            startY += buttonHeight + 2;
//
//        }
//    }
//}
//const FileBrowser& getRootParent(const FileBrowser& file) {
//    if (file.parent != nullptr) {
//        return getRootParent(*file.parent);
//    }
//    return file;
//}
//void moveButtonsExplorer(unordered_map<size_t, FileBrowser>& FilesList, const FileBrowser *file, RollupType type, pair<int, int> expandArea) {
//    if (file == nullptr) {
//        int y{25 - ExplorerScrollbar.nPos};
//        for (auto& el: FilesList) {
//            if (el.second.parent == nullptr) {
//                moveFoldersGroup(FilesList, el.second, y);
//            }
//        }
//        maxYButton = y + ExplorerScrollbar.nPos;
//        return;
//    }
//    if (type == RollupType::EXPAND) {
//        
//        for (auto& child : file->Childrens) {
//            if (FilesList[child].y >= expandArea.first) {
//                moveFoldersGroup(FilesList, FilesList[child], expandArea.second);
//                expandArea.second += buttonHeight + 2;
//            }
//        }
//        moveButtonsExplorer(FilesList, file->parent, RollupType::EXPAND, expandArea);
//    }
//    else if (type == RollupType::COLLAPSE) {
//        for (auto& child : file->Childrens) {
//            if (FilesList[child].y >= expandArea.second) {
//                moveFoldersGroup(FilesList, FilesList[child], expandArea.first);
//                expandArea.first += buttonHeight + 2;
//            }
//        }
//        moveButtonsExplorer(FilesList, file->parent, RollupType::COLLAPSE, expandArea);
//    }
//}
//pair<int, int> CollapseFolder(unordered_map<size_t, FileBrowser>& FilesList, FileBrowser& Folder) {
//    int maxY{ };
//    for (auto& i : Folder.Childrens) {
//        if (!FilesList[i].Childrens.empty()) {
//            pair<int, int>  _ = CollapseFolder(FilesList, FilesList[i]);
//            if (_.second > maxY) {
//                maxY = _.second;
//            }
//        }           
//        ShowWindow(FilesList[i].buttonHWND, SW_HIDE);
//        FilesList[i].isShown = false;
//        if (FilesList[i].y > maxY-2) {
//            maxY = FilesList[i].y + buttonHeight+2;
//        }
//    }
//    return { Folder.y+buttonHeight+2, maxY }; // min, max
//}
//pair<int,int> ExpandFolder(unordered_map<size_t, FileBrowser>& FilesList, FileBrowser& Folder) {
//    cout << "Called Expand" << endl;
//    int maxY{ };
//    if (Folder.Childrens.empty()) {
//        return { 0,0 };
//    }
//    for (auto& i : Folder.Childrens) {
//        if (!FilesList[i].Childrens.empty()) {
//            if (FilesList[i].isDirOpen) {
//                pair<int, int> _ = ExpandFolder(FilesList, FilesList[i]);
//                if (_.second > maxY) {
//                    maxY = _.second;
//                }
//            }
//            
//        }
//        
//        ShowWindow(FilesList[i].buttonHWND, SW_SHOW);
//        FilesList[i].isShown = true;    
//        if (FilesList[i].y > maxY-2) {
//            maxY = FilesList[i].y + buttonHeight+2;
//        }
//    }
//    return {Folder.y+buttonHeight, maxY}; // min, max
//}
//void ListFilesAndFolders(wstring directory, unordered_map<size_t, FileBrowser>& FilesList, HWND hWnd, FileBrowser* parent = nullptr, int spacing = 25) {
//    
//    WIN32_FIND_DATAW findFileData;
//    HANDLE hFind = FindFirstFileW(((directory + L"\\*").c_str()), &findFileData);
//    if (hFind == INVALID_HANDLE_VALUE) {
//        MessageBox(hWnd, L"Error when opening a directory", L"Error", MB_OK);
//        return;
//    }
//    static size_t index{ 1 };
//    int y{ parent ? parent->y + buttonHeight + 2 : 25 };
//    HDC hdc = GetDC(hWnd);
//    RECT textRect = { 0, 0, 0, 0 };
//    
//    do {
//        wstring name = findFileData.cFileName;
//        if (name == L"." || name == L"..") continue;
//        
//        DrawText(hdc, name.c_str(), (int)name.length(), &textRect, DT_CALCRECT);
//        FileBrowser file = FileBrowser{ name, directory+L"\\"+name};
//        int buttonWidth{ textRect.right + 20 };
//        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
//            file.type = FILETYPE::DIR;
//            buttonWidth += 15;
//        }
//        else {
//            file.type = FILETYPE::FILE;
//        }
//        if (parent) {
//            file.parent = parent;
//            parent->Childrens.insert(index);
//            spacing = parent->spacing + 25;
//        }
//        file.buttonMenu = index;
//        file.spacing = spacing;
//        file.y = y;
//        file.buttonHWND = CreateWindow(L"BUTTON", L"",
//            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
//            spacing, y, buttonWidth, buttonHeight,
//            hWnd, (HMENU)index, hInstExplorer, NULL);
//        
//        y += buttonHeight + 2;
//        FilesList[index] = file;
//        index++;
//    } while (FindNextFileW(hFind, &findFileData) != 0);
//    ReleaseDC(hWnd, hdc);
//    FindClose(hFind);
//}



void AddTreeViewItems(HWND hwndTV, wstring directory, HTREEITEM hParent = NULL) {
    if (directory.empty()) {
        TVINSERTSTRUCT tvi = {};
        tvi.hParent = hParent;
        tvi.hInsertAfter = TVI_LAST;
        tvi.item.mask = TVIF_TEXT;
        tvi.item.pszText = (LPWSTR)L"Select directory";
        TreeView_InsertItem(hwndTV, &tvi);
        return;
    }
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((directory + L"\\*").c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        wstring name = findFileData.cFileName;
        if (name == L"." || name == L"..") continue;

        TVINSERTSTRUCT tvi = {};
        tvi.hParent = hParent;
        tvi.hInsertAfter = TVI_LAST;
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvi.item.pszText = (LPWSTR)name.c_str();
        tvi.item.lParam = (LPARAM)new wstring(directory + L"\\" + name);

        HTREEITEM hNewItem = TreeView_InsertItem(hwndTV, &tvi);
        treeItemPaths[hNewItem] = directory + L"\\" + name;
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            TVINSERTSTRUCT tviChild = {};
            tviChild.hParent = hNewItem;
            tviChild.hInsertAfter = TVI_LAST;
            tviChild.item.mask = TVIF_TEXT;
            tviChild.item.pszText = (LPWSTR)L"Loading...";

            TreeView_InsertItem(hwndTV, &tviChild);
        }
    } while (FindNextFileW(hFind, &findFileData));
    FindClose(hFind);
}
void updateFileExplorerOnChangeDir(HWND hWnd, wstring directory) {
    TreeView_DeleteAllItems(hTreeView);
    AddTreeViewItems(hTreeView, directory);
    UpdateWindow(hTreeView);
}
LRESULT CALLBACK FileExplorerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    FileManager& manager = FileManager::getInstance();
    RECT rt;
    GetClientRect(hWnd, &rt);
    switch (message)
    {
    case WM_CREATE: {
        manager.registerOnChangeDirectory(updateFileExplorerOnChangeDir, hWnd);
        hTreeView = CreateWindowEx(0, WC_TREEVIEW, L"", WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_TRACKSELECT | TVS_LINESATROOT | TVS_HASBUTTONS,
            0, 0, rt.right, rt.bottom, hWnd, (HMENU)1, hInstExplorer, NULL);
        AddTreeViewItems(hTreeView, L"");
        TreeView_SetBkColor(hTreeView, RGB(38, 40, 49));
        TreeView_SetTextColor(hTreeView, RGB(255, 255, 255));
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        switch (nmhdr->code) {
            case TVN_ITEMEXPANDING: {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                wstring* path = (wstring*)pnmtv->itemNew.lParam;
                if (path) {
                    wcout << *path << endl;

                    HTREEITEM hChild = TreeView_GetChild(hTreeView, pnmtv->itemNew.hItem);
                    while (hChild) {
                        HTREEITEM hNext = TreeView_GetNextSibling(hTreeView, hChild);
                        TreeView_DeleteItem(hTreeView, hChild);
                        hChild = hNext;
                    }

                    AddTreeViewItems(hTreeView, *path, pnmtv->itemNew.hItem);
                    UpdateWindow(hTreeView);
                }
                break;
            }
            case NM_DBLCLK: {
                DWORD dwPos = GetMessagePos();
                POINT pt;
                pt.x = GET_X_LPARAM(dwPos);
                pt.y = GET_Y_LPARAM(dwPos);
                ScreenToClient(hTreeView, &pt);

                TVHITTESTINFO ht = { 0 };
                ht.pt = pt;
                HTREEITEM hItem = TreeView_HitTest(hTreeView, &ht);
                if (ht.flags & TVHT_ONITEM) {
                    TVITEM tvi = { 0 };
                    tvi.hItem = hItem;
                    tvi.mask = TVIF_PARAM;
                    TreeView_GetItem(hTreeView, &tvi);
                    wstring* path = (wstring*)tvi.lParam;
                    if (path) {
                        wcout << L"Clicked on: " << *path << endl;
                        DWORD attributes = GetFileAttributesW(path->c_str());
                        if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            manager.openFile(*path);
                        }
                    }
                }
                break;
            }
            case TVN_DELETEITEM: {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                wstring* path = reinterpret_cast<wstring*>(pnmtv->itemOld.lParam);
                delete path;  
                treeItemPaths.erase(pnmtv->itemOld.hItem);
                break;
            }
        }
        break;
    }
    
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED) {
            SetFocus(hWnd);
        }
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
       
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    }
    //case WM_MOUSEACTIVATE: {
    //    cout << "FileExplorer WM_MOUSEACTIVATE" << endl;
    //    return MA_ACTIVATE;
    //}
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}