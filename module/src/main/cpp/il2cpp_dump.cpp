#include "il2cpp_dump.h"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include "log.h"
#include "il2cpp-tabledefs.h"
#include "il2cpp-class.h"

#define DO_API(r, n, p) r (*n) p
#include "il2cpp-api-functions.h"
#undef DO_API

static uint64_t il2cpp_base = 0;

// 改进符号查找逻辑，不再依赖外部 xdl 库
void init_il2cpp_api(void *handle) {
#define DO_API(r, n, p) {                      \
    n = (r (*) p)dlsym(handle, #n);             \
    if(!n) {                                    \
        LOGW("API missing: %s", #n);            \
    }                                           \
}
#include "il2cpp-api-functions.h"
#undef DO_API
}

// ... 此处保留原有的 get_method_modifier, _il2cpp_type_is_byref 等辅助函数 ...
// (因逻辑相同，为了篇幅略过，请在实际文件中保留它们)

// [保持原有 get_method_modifier, dump_method, dump_property, dump_field, dump_type 函数不变]

void il2cpp_api_init(void *handle) {
    LOGI("Initializing IL2CPP APIs...");
    init_il2cpp_api(handle);

    if (il2cpp_domain_get_assemblies) {
        Dl_info dlInfo;
        if (dladdr((void *) il2cpp_domain_get_assemblies, &dlInfo)) {
            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
        }
        LOGI("il2cpp_base: %" PRIx64"", il2cpp_base);
    } else {
        LOGE("Failed to initialize critical APIs.");
        return;
    }

    // 适配最新 Unity 引擎的 VM 检查逻辑
    while (il2cpp_is_vm_thread && !il2cpp_is_vm_thread(nullptr)) {
        LOGI("Waiting for VM thread...");
        sleep(1);
    }

    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
}

void il2cpp_dump(const char *outDir) {
    LOGI("Dump process started...");
    
    // 确保输出目录存在
    std::string filesDir = std::string(outDir) + "/files";
    mkdir(filesDir.c_str(), 0700);
    std::string outPath = filesDir + "/dump.cs";
    
    std::ofstream outStream(outPath);
    if (!outStream.is_open()) {
        LOGE("Cannot open output file: %s", outPath.c_str());
        return;
    }

    size_t size;
    auto domain = il2cpp_domain_get();
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);

    LOGI("Found %zu assemblies", size);

    // 1. 写入 Header 信息
    outStream << "// Dumped by Zygisk-Il2CppDumper (Updated)\n";
    outStream << "// Base: 0x" << std::hex << il2cpp_base << "\n\n";

    // 2. 遍历 Assembly
    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        const char* imageName = il2cpp_image_get_name(image);
        LOGI("Processing assembly: %s", imageName);
        
        outStream << "// Dll: " << imageName << "\n";

        if (il2cpp_image_get_class) {
            auto classCount = il2cpp_image_get_class_count(image);
            for (int j = 0; j < classCount; ++j) {
                auto klass = il2cpp_image_get_class(image, j);
                auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
                outStream << dump_type(type);
            }
        } else {
            // 旧版本 Unity 反射逻辑 (保留兼容性)
            // ... [此处插入原有的 Reflection 遍历逻辑] ...
        }
        outStream.flush(); // 及时写入磁盘，防止大文件丢失
    }

    outStream.close();
    LOGI("Dump successful! Path: %s", outPath.c_str());
}
