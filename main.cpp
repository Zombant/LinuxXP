#include <cstdlib>
#include <cstdio>
#include "window_manager.hpp"

using ::std::unique_ptr;

int main(int argc, char** argv) {
    unique_ptr<WindowManager> window_manager = WindowManager::Create();
    if(!window_manager) {
        printf("Failed to start window manager");
        return 1;
    }

    window_manager->Start();

    return 0;
}
