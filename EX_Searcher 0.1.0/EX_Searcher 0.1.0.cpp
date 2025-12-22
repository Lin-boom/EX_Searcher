#include <windows.h>
#include <string>
#include <shellapi.h>

HWND hFolderEdit, hExtEdit, hListBox;
HWND hSearchButton, hOpenFolderButton, hOpenFileButton, hDeleteFileButton;

/* ================= 遞迴搜尋 ================= */
void SearchFilesRecursive(const std::string& folder, const std::string& ext) {
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile((folder + "\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string name = fd.cFileName;
        std::string full = folder + "\\" + name;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (name != "." && name != "..") {
                SearchFilesRecursive(full, ext);
            }
        } else {
            size_t p = name.rfind('.');
            if (p != std::string::npos && name.substr(p) == ext) {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)full.c_str());
            }
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
}

/* ================= 控制元件自適應 ================= */
void ResizeControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    int w = rc.right;
    int h = rc.bottom;

    int margin = 15;
    int spacing = 8;
    int btnH = 26;
    int btnW = 100;
    int extW = 80;

    // 右側四個按鈕（刪除在最右）
    int xDelete = w - margin - btnW;
    int xOpenFile = xDelete - spacing - btnW;
    int xOpenFolder = xOpenFile - spacing - btnW;
    int xSearch = xOpenFolder - spacing - btnW;

    int xExt = xSearch - spacing - extW;

    MoveWindow(hFolderEdit, margin, 15, xExt - margin, btnH, TRUE);
    MoveWindow(hExtEdit, xExt, 15, extW, btnH, TRUE);

    MoveWindow(hSearchButton, xSearch, 15, btnW, btnH, TRUE);
    MoveWindow(hOpenFolderButton, xOpenFolder, 15, btnW, btnH, TRUE);
    MoveWindow(hOpenFileButton, xOpenFile, 15, btnW, btnH, TRUE);
    MoveWindow(hDeleteFileButton, xDelete, 15, btnW, btnH, TRUE);

    MoveWindow(hListBox, margin, 55, w - margin * 2, h - 70, TRUE);
}

/* ================= 搜尋按鈕啟用 ================= */
void UpdateSearchButtonState() {
    char buf[MAX_PATH];
    GetWindowText(hFolderEdit, buf, MAX_PATH);
    EnableWindow(hSearchButton, strlen(buf) > 0);
}

/* ================= 視窗程序 ================= */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        hFolderEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            0,0,0,0, hwnd, NULL, NULL, NULL);

        hExtEdit = CreateWindow("EDIT", ".txt", WS_CHILD | WS_VISIBLE | WS_BORDER,
            0,0,0,0, hwnd, NULL, NULL, NULL);

        hSearchButton = CreateWindow("BUTTON", "搜尋", WS_CHILD | WS_VISIBLE,
            0,0,0,0, hwnd, (HMENU)1, NULL, NULL);

        hOpenFolderButton = CreateWindow("BUTTON", "開啟位置", WS_CHILD | WS_VISIBLE,
            0,0,0,0, hwnd, (HMENU)2, NULL, NULL);

        hOpenFileButton = CreateWindow("BUTTON", "開啟檔案", WS_CHILD | WS_VISIBLE,
            0,0,0,0, hwnd, (HMENU)3, NULL, NULL);

        hDeleteFileButton = CreateWindow("BUTTON", "刪除檔案", WS_CHILD | WS_VISIBLE,
            0,0,0,0, hwnd, (HMENU)4, NULL, NULL);

        hListBox = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
            0,0,0,0, hwnd, NULL, NULL, NULL);

        DragAcceptFiles(hwnd, TRUE);
        UpdateSearchButtonState();
        break;

    case WM_SIZE:
        ResizeControls(hwnd);
        break;

    case WM_COMMAND:
        if (HIWORD(wp) == EN_CHANGE && (HWND)lp == hFolderEdit)
            UpdateSearchButtonState();

        switch (LOWORD(wp)) {
        case 1: { // 搜尋
            char folder[MAX_PATH], ext[32];
            GetWindowText(hFolderEdit, folder, MAX_PATH);
            GetWindowText(hExtEdit, ext, 32);
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            SearchFilesRecursive(folder, ext);
            break;
        }
        case 2: { // 開啟檔案位置
            int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char path[MAX_PATH];
                SendMessage(hListBox, LB_GETTEXT, sel, (LPARAM)path);
                char cmd[MAX_PATH + 20];
                wsprintf(cmd, "/select,\"%s\"", path);
                ShellExecute(NULL, "open", "explorer.exe", cmd, NULL, SW_SHOW);
            }
            break;
        }
        case 3: { // 開啟檔案
            int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char path[MAX_PATH];
                SendMessage(hListBox, LB_GETTEXT, sel, (LPARAM)path);
                ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOW);
            }
            break;
        }
        case 4: { // 刪除檔案
            int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                char path[MAX_PATH];
                SendMessage(hListBox, LB_GETTEXT, sel, (LPARAM)path);
                if (MessageBox(hwnd, "確定刪除檔案？", "確認",
                    MB_YESNO | MB_ICONWARNING) == IDYES) {
                    if (DeleteFile(path)) {
                        SendMessage(hListBox, LB_DELETESTRING, sel, 0);
                    } else {
                        MessageBox(hwnd, "刪除失敗", "錯誤", MB_OK | MB_ICONERROR);
                    }
                }
            }
            break;
        }
        }
        break;

    case WM_DROPFILES: {
        HDROP h = (HDROP)wp;
        char path[MAX_PATH];
        DragQueryFile(h, 0, path, MAX_PATH);
        DragFinish(h);
        SetWindowText(hFolderEdit, path);
        UpdateSearchButtonState();
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

/* ================= WinMain ================= */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "FileSearcher";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("FileSearcher", "檔案搜尋器",
        WS_OVERLAPPEDWINDOW, 100, 100, 920, 520,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

