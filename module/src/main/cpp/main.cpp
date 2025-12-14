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

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        const char *app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        if (process_name && is_target_game(process_name)) {
            LOGI("Target detected: %s", process_name);
            enable_hack = true;
            game_data_dir = strdup(app_data_dir);
            api->setOption(zygisk::Option::FORCE_DENYLIST_UNMOUNT);
        } else {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }

        if (process_name) env->ReleaseStringUTFChars(args->nice_name, process_name);
        if (app_data_dir) env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            std::thread([this]() {
                sleep(8); 
                hack_prepare(game_data_dir, nullptr, 0); 
                if (game_data_dir) free(game_data_dir);
            }).detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack = false;
    char *game_data_dir = nullptr;

    bool is_target_game(const char *current_process) {
        std::ifstream cfg("/data/local/tmp/il2cpp.cfg");
        if (!cfg.is_open()) return false;
        std::string target;
        if (std::getline(cfg, target)) {
            target.erase(0, target.find_first_not_of(" \n\r\t"));
            target.erase(target.find_last_not_of(" \n\r\t") + 1);
            return target == current_process;
        }
        return false;
    }
};

REGISTER_ZYGISK_MODULE(MyModule)
