#include "../include/gui_app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    YoutubeCore::GuiApp app(hInstance);
    return app.Run(nCmdShow);
}
