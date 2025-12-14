#include "il2cpp_dump.h"
#include <dlfcn.h>
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

// --- 辅助函数实现 ---

std::string get_method_modifier(uint32_t flags) {
    std::stringstream outPut;
    auto access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access) {
        case METHOD_ATTRIBUTE_PRIVATE: outPut << "private "; break;
        case METHOD_ATTRIBUTE_PUBLIC: outPut << "public "; break;
        case METHOD_ATTRIBUTE_FAMILY: outPut << "protected "; break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: outPut << "internal "; break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM: outPut << "protected internal "; break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC) outPut << "static ";
    if (flags & METHOD_ATTRIBUTE_VIRTUAL) outPut << "virtual ";
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) outPut << "abstract ";
    return outPut.str();
}

bool _il2cpp_type_is_byref(const Il2CppType *type) {
    return type->byref;
}

std::string dump_field(Il2CppClass *klass) {
    std::stringstream outPut;
    outPut << "\n\t// Fields\n";
    void *iter = nullptr;
    while (auto field = il2cpp_class_get_fields(klass, &iter)) {
        outPut << "\t";
        auto attrs = il2cpp_field_get_flags(field);
        auto access = attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
        if (access == FIELD_ATTRIBUTE_PUBLIC) outPut << "public ";
        else outPut << "private ";
        if (attrs & FIELD_ATTRIBUTE_STATIC) outPut << "static ";
        
        auto field_type = il2cpp_field_get_type(field);
        auto field_class = il2cpp_class_from_type(field_type);
        outPut << il2cpp_class_get_name(field_class) << " " << il2cpp_field_get_name(field);
        outPut << "; // 0x" << std::hex << il2cpp_field_get_offset(field) << "\n";
    }
    return outPut.str();
}

std::string dump_method(Il2CppClass *klass) {
    std::stringstream outPut;
    outPut << "\n\t// Methods\n";
    void *iter = nullptr;
    while (auto method = il2cpp_class_get_methods(klass, &iter)) {
        if (method->methodPointer) {
            outPut << "\t// RVA: 0x" << std::hex << (uint64_t)method->methodPointer - il2cpp_base;
        }
        outPut << "\n\t" << get_method_modifier(il2cpp_method_get_flags(method, nullptr));
        auto return_type = il2cpp_method_get_return_type(method);
        auto return_class = il2cpp_class_from_type(return_type);
        outPut << il2cpp_class_get_name(return_class) << " " << il2cpp_method_get_name(method) << "(";
        // 参数简化处理
        outPut << ");\n";
    }
    return outPut.str();
}

std::string dump_type(const Il2CppType *type) {
    std::stringstream outPut;
    auto *klass = il2cpp_class_from_type(type);
    outPut << "\n// Namespace: " << il2cpp_class_get_namespace(klass) << "\n";
    outPut << "class " << il2cpp_class_get_name(klass) << "\n{";
    outPut << dump_field(klass);
    outPut << dump_method(klass);
    outPut << "}\n";
    return outPut.str();
}

// --- 导出接口 ---

void il2cpp_api_init(void *handle) {
#define DO_API(r, n, p) { n = (r (*) p)dlsym(handle, #n); }
#include "il2cpp-api-functions.h"
#undef DO_API

    if (il2cpp_domain_get_assemblies) {
        Dl_info dlInfo;
        if (dladdr((void *) il2cpp_domain_get_assemblies, &dlInfo)) {
            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
        }
    }
    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
}

void il2cpp_dump(const char *outDir) {
    LOGI("Dumping process started...");
    size_t size;
    auto domain = il2cpp_domain_get();
    auto assemblies = il2cpp_domain_get_assemblies(domain, &size);

    std::string filesDir = std::string(outDir) + "/files";
    mkdir(filesDir.c_str(), 0777);
    std::string outPath = filesDir + "/dump.cs";

    std::ofstream outStream(outPath);
    outStream << "// Base: 0x" << std::hex << il2cpp_base << "\n";

    for (int i = 0; i < size; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            auto klass = il2cpp_image_get_class(image, j);
            if (!klass) continue;
            auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
            outStream << dump_type(type);
        }
        outStream.flush();
    }
    outStream.close();
    LOGI("Dump success: %s", outPath.c_str());
}
