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

// --- 前向声明 ---
std::string get_method_modifier(uint32_t flags);
bool _il2cpp_type_is_byref(const Il2CppType *type);
std::string dump_method(Il2CppClass *klass);
std::string dump_property(Il2CppClass *klass);
std::string dump_field(Il2CppClass *klass);
std::string dump_type(const Il2CppType *type);
void init_il2cpp_api(void *handle);

// --- 核心实现 ---
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

// [保留你原本代码中 get_method_modifier, _il2cpp_type_is_byref 等所有辅助函数]
// 注意：确保这些函数名和逻辑都在这里完整实现

void il2cpp_api_init(void *handle) {
    LOGI("il2cpp_handle: %p", handle);
    init_il2cpp_api(handle);
    if (il2cpp_domain_get_assemblies) {
        Dl_info dlInfo;
        if (dladdr((void *) il2cpp_domain_get_assemblies, &dlInfo)) {
            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
        }
        LOGI("il2cpp_base: %" PRIx64"", il2cpp_base);
    } else {
        LOGE("Failed to initialize il2cpp api.");
        return;
    }
    while (il2cpp_is_vm_thread && !il2cpp_is_vm_thread(nullptr)) {
        LOGI("Waiting for il2cpp_init...");
        sleep(1);
    }
    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
}

void il2cpp_dump(const char *outDir) {
    LOGI("dumping...");
    size_t size;
    auto domain = il2cpp_domain_get();
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);
    
    // 确保输出目录存在
    std::string filesDir = std::string(outDir) + "/files";
    mkdir(filesDir.c_str(), 0777);
    std::string outPath = filesDir + "/dump.cs";
    
    std::ofstream outStream(outPath);
    if (!outStream.is_open()) {
        LOGE("Cannot write to %s", outPath.c_str());
        return;
    }

    outStream << "// Base: 0x" << std::hex << il2cpp_base << "\n";

    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        outStream << "\n// Image " << i << ": " << il2cpp_image_get_name(image) << "\n";
        
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            auto klass = il2cpp_image_get_class(image, j);
            auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
            outStream << dump_type(type);
        }
        outStream.flush();
    }
    outStream.close();
    LOGI("Done! File saved to %s", outPath.c_str());
}
