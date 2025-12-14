#!/system/bin/sh

ui_print "--------------------------------------"
ui_print "    Zygisk-Il2CppDumper (V2 Pro)"
ui_print "--------------------------------------"

# 定义路径
CFG_DIR="/data/local/tmp"
CFG_FILE="il2cpp.cfg"
CFG_PATH="$CFG_DIR/$CFG_FILE"

# 1. 自动搜寻设备上的 Unity/IL2CPP 游戏
ui_print "- 正在自动扫描设备上的 Unity 游戏..."
UNITY_GAMES=""

# 遍历所有已安装的第三方应用包名
for pkg in $(pm list packages -3 | cut -f 2 -d ":"); do
    # 获取应用的库目录
    LIB_PATH=$(pm dump $pkg | grep "nativeLibraryDir" | sed 's/.*nativeLibraryDir=//;s/ .*//' | xargs)
    
    # 检查是否存在 libil2cpp.so
    if [ -f "$LIB_PATH/libil2cpp.so" ]; then
        ui_print "  [!] 发现目标: $pkg"
        UNITY_GAMES="$UNITY_GAMES $pkg"
    fi
done

if [ -z "$UNITY_GAMES" ]; then
    ui_print "- 未发现明显的 Unity 游戏，请稍后手动配置。"
fi

# 2. 配置文件处理
mkdir -p $CFG_DIR

if [ -f "$CFG_PATH" ]; then
    ui_print "- 保持现有配置: $(cat $CFG_PATH)"
else
    # 如果没有配置且搜寻到了游戏，默认填入搜寻到的第一个
    if [ ! -z "$UNITY_GAMES" ]; then
        # 提取第一个包名
        FIRST_GAME=$(echo $UNITY_GAMES | cut -d' ' -f1)
        echo "$FIRST_GAME" > "$CFG_PATH"
        ui_print "- 已自动将目标指向: $FIRST_GAME"
    else
        echo "com.example.game" > "$CFG_PATH"
        ui_print "- 已创建空白模板，请稍后自行修改。"
    fi
fi

# 3. 权限修正 (Magisk 内部函数)
set_perm $CFG_PATH 0 0 0777

ui_print "--------------------------------------"
ui_print "  安装完成！"
ui_print "  配置文件: $CFG_PATH"
ui_print "  Dump 结果将保存在游戏的 files 目录下"
ui_print "--------------------------------------"
