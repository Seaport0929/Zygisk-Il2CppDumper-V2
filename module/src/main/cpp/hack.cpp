#include "hack.h"
#include "il2cpp_dump.h"
#include "log.h"
#include <unistd.h>
#include <dlfcn.h>
#include <thread>

void perform_dump(const char *game_data_dir) {
    LOGI("Checking for libil2cpp.so...");
    void *handle = nullptr;
    for (int i = 0; i < 30; i++) {
        handle = dlopen("libil2cpp.so", RTLD_LAZY);
        if (handle) break;
        sleep(1);
    }

    if (handle) {
        LOGI("libil2cpp.so found, initializing...");
        il2cpp_api_init(handle);
        il2cpp_dump(game_data_dir);
    } else {
        LOGE("libil2cpp.so NOT found. Is this an IL2CPP game?");
    }
}

void hack_prepare(const char *game_data_dir, void *data, size_t length) {
    LOGI("Hack thread starting for: %s", game_data_dir);
    std::thread(perform_dump, game_data_dir).detach();
}
