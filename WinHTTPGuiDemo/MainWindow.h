// MainWindow.h
#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <lua/lua.hpp>

class MainWindow {
public:
    MainWindow(HINSTANCE hInst);
    ~MainWindow();

    // 注册窗口类并创建窗口
    bool Initialize(int nCmdShow);
    // 进入消息循环
    int  Run();

private:
    // 窗口相关
    HINSTANCE           hInst_;
    HWND                hwnd_;
    static constexpr wchar_t CLASS_NAME_[] = L"MyHttpSenderClass";

    // 字体
    HFONT               font_;

    // Lua 和按钮回调映射
    lua_State*          L_;
    std::map<int,std::string> btnCallbacks_;

    // 控件句柄
    HWND hUrlEdit_, hBrowseBtn_, hFileEdit_;
    HWND hSendBtn_, hClearBtn_, hLogEdit_;
    HWND hDescStatic_;
	std::wstring descText_;
    // 私有方法
    void    RegisterWindowClass();
    void    CreateWindowInstance(int nCmdShow);
    void    CreateControls();
    void    LoadButtonsFromLua();
    void    AppendLog(const std::wstring& text);
    // —— 新增 HTTP 逻辑 —— 
    void          DoHttpRequest(const std::wstring& url, const std::wstring& body);

    static  LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM);
    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    // 消息处理
    LRESULT OnCreate();
    LRESULT OnCommand(WPARAM wParam);
    LRESULT OnDestroy();
};


