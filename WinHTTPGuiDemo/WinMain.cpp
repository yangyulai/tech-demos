// WinMain.cpp
#include "MainWindow.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    MainWindow wnd(hInst);
    if (!wnd.Initialize(nCmdShow)) return -1;
    return wnd.Run();
}
