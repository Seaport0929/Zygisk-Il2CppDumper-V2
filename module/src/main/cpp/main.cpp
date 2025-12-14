#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include "hack.h"
#include "zygisk.hpp"
#include "log.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    // 在应用进程启动前触发
    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        const char *app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        if (process_name && is_target_game(process_name)) {
            LOGI("Target detected: %s", process_name);
            enable_hack = true;
            
            // 记录数据目录以便稍后存放 dump 文件
            game_data_dir = strdup(app_data_dir);
            
            // 现代 Zygisk 建议在这里设置卸载选项
            // 某些游戏会扫描内存中的模块，dump 完后我们会尝试卸载
            api->setOption(zygisk::Option::FORCE_DENYLIST_UNMOUNT);
        } else {
            // 非目标应用直接卸载模块，保持环境纯净
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }

        env->ReleaseStringUTFChars(args->nice_name, process_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    // 在应用进程环境初始化完成后触发
    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            // 核心改进：开启独立线程并延迟执行
            // 1. 避免阻塞主线程导致游戏启动卡死
            // 2. 等待 libil2cpp.so 载入内存
            std::thread([this]() {
                LOGI("Wait for game initialization...");
                sleep(8); // 延迟 8 秒，确保大多数重度游戏完成解密载入
                hack_prepare(game_data_dir, nullptr, 0); 
                LOGI("Dump process completed.");
            }).detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack = false;
    char *game_data_dir = nullptr;

    // 动态检查配置文件
    bool is_target_game(const char *current_process) {
        // 配置文件路径：/data/local/tmp/il2cpp.cfg
        std::ifstream cfg("/data/local/tmp/il2cpp.cfg");
        if (!cfg.is_open()) return false;

        std::string target;
        if (std::getline(cfg, target)) {
            // 去除末尾的换行符或空格
            target.erase(target.find_last_not_of(" \n\r\t") + 1);
            return target == current_process;
        }
        return false;
    }
};

REGISTER_ZYGISK_MODULE(MyModule)
