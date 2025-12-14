#include "hack.h"
#include "il2cpp_dump.h"
#include "log.h"
#include <unistd.h>
#include <thread>
#include <dlfcn.h>
#include <string>

// 真正执行 Dump 的逻辑
void perform_dump(const char *game_data_dir) {
    LOGI("Dump thread started, PID: %d", getpid());
    
    void *handle = nullptr;
    int retry_count = 0;
    const int max_retries = 20; // 最多等待 20 秒

    // 循环检查 libil2cpp.so 是否已加载
    while (retry_count < max_retries) {
        handle = dlopen("libil2cpp.so", RTLD_LAZY);
        if (handle) {
            LOGI("libil2cpp.so found at retry %d", retry_count);
            break;
        }
        retry_count++;
        sleep(1);
    }

    if (handle) {
        // 初始化 API 并执行 Dump
        // 注意：il2cpp_api_init 和 il2cpp_dump 定义在 il2cpp_dump.cpp 中
        il2cpp_api_init(handle);
        il2cpp_dump(game_data_dir);
        LOGI("Dump finished. Check your files directory.");
    } else {
        LOGE("libil2cpp.so not found after %d seconds. Is this an IL2CPP game?", max_retries);
    }
}

// 统一入口，由 main.cpp 调用
void hack_prepare(const char *game_data_dir, void *data, size_t length) {
    LOGI("Preparing hack for: %s", game_data_dir);

    // 开启独立线程执行，避免阻塞游戏主逻辑
    std::thread dump_thread(perform_dump, game_data_dir);
    dump_thread.detach();
}

// 现代真机不需要此复杂的 JNI_OnLoad 桥接逻辑，统一由 main.cpp 处理即可
