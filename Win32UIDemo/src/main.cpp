// main.cpp - C++17 Win32 UI 示例
#define UNICODE
#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#pragma comment(lib, "comctl32.lib")

// 控件 ID
enum {
    ID_LOG = 101,
    ID_LIST = 102,
    ID_EDIT = 103,
    ID_SEND = 104
};

// 自定义消息：将日志追加到日志区
static constexpr UINT WM_LOG_APPEND = WM_APP + 1;

HWND hwndMain{}, hwndLog{}, hwndList{}, hwndEdit{}, hwndSendBtn{}, hwndStatus{};
std::atomic<bool> running{ true };

// 将一行文本异步追加到日志区
void PostLog(std::wstring text) {
    // 在堆上分配字符串，PostMessage 后主线程释放
    auto p = new std::wstring(std::move(text));
    PostMessage(hwndMain, WM_LOG_APPEND, 0, reinterpret_cast<LPARAM>(p));
}

// 后台线程：每 2 秒写一条日志
void BackgroundThread() {
    using namespace std::chrono_literals;
    while (running) {
        PostLog(L"[BG] Background event occurred\n");
        std::this_thread::sleep_for(2000ms);
    }
}

// 窗口消息过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // 启用公用控件
        INITCOMMONCONTROLSEX icex{ sizeof(icex), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES };
        InitCommonControlsEx(&icex);

        // 初次创建时获取客户区大小
        RECT rc; GetClientRect(hWnd, &rc);
        int width = rc.right, height = rc.bottom;
        int statusH = 22, inputH = 24;
        int listW = width / 3;
        int logW = width - listW;
        int contentH = height - statusH - inputH;

        // 日志区：只读多行编辑框
        hwndLog = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, logW, contentH,
            hWnd, (HMENU)ID_LOG, nullptr, nullptr);

        // 列表区：Report 模式 ListView
        hwndList = CreateWindowEx(
            WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            logW, 0, listW, contentH,
            hWnd, (HMENU)ID_LIST, nullptr, nullptr);
        // 添加一列
        LVCOLUMN lvc{ LVCF_TEXT | LVCF_WIDTH, 0 };
        lvc.cx = listW - 4; lvc.pszText = const_cast<LPWSTR>(L"Items");
        ListView_InsertColumn(hwndList, 0, &lvc);

        // 命令输入框
        hwndEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, contentH, width - 80, inputH,
            hWnd, (HMENU)ID_EDIT, nullptr, nullptr);

        // “Send” 按钮
        hwndSendBtn = CreateWindow(
            L"BUTTON", L"Send",
            WS_CHILD | WS_VISIBLE,
            width - 80, contentH, 80, inputH,
            hWnd, (HMENU)ID_SEND, nullptr, nullptr);

        // 状态栏
        hwndStatus = CreateWindowEx(
            0, STATUSCLASSNAME, nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hWnd, nullptr, nullptr, nullptr);
        // 两个部分：状态文本 + 连接数
        int parts[2] = { width * 3 / 4, -1 };
        SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts);
        SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)L"Status: Running");
        SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)L"Count: 0");
    } return 0;

    case WM_SIZE: {
        // 调整子控件布局
        int width = LOWORD(lp), height = HIWORD(lp);
        int statusH = 22, inputH = 24;
        int listW = width / 3;
        int logW = width - listW;
        int contentH = height - statusH - inputH;

        MoveWindow(hwndLog, 0, 0, logW, contentH, TRUE);
        MoveWindow(hwndList, logW, 0, listW, contentH, TRUE);
        MoveWindow(hwndEdit, 0, contentH, width - 80, inputH, TRUE);
        MoveWindow(hwndSendBtn, width - 80, contentH, 80, inputH, TRUE);
        MoveWindow(hwndStatus, 0, height - statusH, width, statusH, TRUE);

        // 更新状态栏分段
        int parts[2] = { width * 3 / 4, -1 };
        SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts);
    } return 0;

    case WM_LOG_APPEND: {
        // 在主线程追加日志文本
        auto pStr = reinterpret_cast<std::wstring*>(lp);
        int len = GetWindowTextLengthW(hwndLog);
        SendMessage(hwndLog, EM_SETSEL, len, len);
        SendMessage(hwndLog, EM_REPLACESEL, FALSE, (LPARAM)pStr->c_str());
        delete pStr;

        // 更新状态栏中“Count”值为日志行数
        int lines = SendMessage(hwndLog, EM_GETLINECOUNT, 0, 0);
        wchar_t buf[64];
        wsprintf(buf, L"Count: %d", lines);
        SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)buf);
    } return 0;

    case WM_COMMAND: {
        if (LOWORD(wp) == ID_SEND) {
            // 处理 Send 按钮：读取命令并执行
            wchar_t cmd[256];
            GetWindowTextW(hwndEdit, cmd, _countof(cmd));
            // 清空输入框
            SetWindowText(hwndEdit, L"");

            std::wstring s(cmd);
            if (s.rfind(L"add ", 0) == 0) {
                // add <text>
                std::wstring item = s.substr(4);
                int idx = ListView_GetItemCount(hwndList);
                LVITEM lvi{ 0 };
                lvi.iItem = idx;
                lvi.mask = LVIF_TEXT;
                lvi.pszText = item.data();
                ListView_InsertItem(hwndList, &lvi);
                PostLog(L"[CMD] Added item: " + item + L"\n");
            }
            else if (s == L"clearlog") {
                SetWindowText(hwndLog, L"");
                PostLog(L"[CMD] Log cleared\n");
            }
            else if (s == L"clearlist") {
                ListView_DeleteAllItems(hwndList);
                PostLog(L"[CMD] List cleared\n");
            }
            else if (s == L"exit") {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            else {
                PostLog(L"[CMD] Unknown command: " + s + L"\n");
            }
        }
    } return 0;

    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    // 注册主窗口类
    WNDCLASSEX wc{ sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"Win32UIDemoClass";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassEx(&wc);

    // 创建主窗口
    hwndMain = CreateWindowEx(
        0, wc.lpszClassName, L"C++17 Win32 UI Demo",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    // 启动后台线程
    std::thread bg{ BackgroundThread };

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 等待后台线程退出
    bg.join();
    return 0;
}
