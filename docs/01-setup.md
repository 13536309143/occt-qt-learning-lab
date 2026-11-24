# 01：双平台安装、构建和启动

## 1. 两个平台相同和不同的部分

建模算法、TopoDS、AIS、Qt 按钮和信号槽可以共享。平台差异集中在编译器、架构、原生窗口和动态库查找。

| 项目 | macOS | Windows |
|---|---|---|
| 编译器 | Apple Clang | MSVC 2022 x64 |
| 原生窗口包装 | Cocoa_Window，NSView | WNT_Window，HWND |
| 库文件 | dylib | DLL / LIB |
| 运行产物 | .app bundle | .exe |
| 初始架构 | 与本机一致，M 系列用 arm64 | 本教程用 x64 |

不要复制 Mac 的构建目录到 Windows，也不要把 MSVC 库与 MinGW 项目混用。

## 2. 本教程首选：仓库内独立依赖环境

本次 Mac 实验没有预先安装 Qt、CMake 或 Homebrew，因此实际采用 micromamba + conda-forge。脚本将其安装到 `.local/`，可以与原有开发环境共存。

固定核心依赖：OCCT 7.9.3、Qt 6.8.4。其余传递依赖由包管理器解决。本次 Mac 的精确包 URL 列表在 `reports/macos-2026-11-24/environment-osx-arm64.explicit.txt`，它只适用于同平台，不能拿去安装 Windows。

### Mac

```bash
xcode-select --install
bash scripts/setup-macos.sh
bash scripts/build-macos.sh
open build-mac/OcctQtStarter.app
```

若系统已装 Xcode，确认 `xcode-select -p` 返回有效开发者目录。构建脚本同时运行建模和 GUI 实验，需要登录桌面，不应通过无显示会话运行 GUI 实验。

### Windows

1. 安装 Git for Windows。
2. 安装 VS 2022 的“使用 C++ 的桌面开发”，包括 Windows SDK 与 MSVC。
3. 将仓库克隆到较短路径，如 `C:\dev\OCCT`。首次避免很长的目录路径。
4. 在该目录运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\run-windows.ps1
```

安装脚本联网下载 micromamba 并创建 `.local\env`；构建脚本寻找 VS 的 `vcvars64.bat`，导入 x64 编译环境，设置 Qt/OCCT 路径并运行 CTest。

没有桌面会话时可以跳过 GUI：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -SkipGui
```

这只能证明编译与建模检查，不能算作完成窗口实验。

## 3. 想手动使用官方 Qt / OCCT SDK

Windows：安装 Qt 6 的 MSVC 2022 64-bit 版本，从 OCCT 7.9.3 Release 下载普通分库 `opencascade-7.9.3-vc14-64.zip` 及需要的第三方库。工程链接独立 TK* 库，不使用 combined 单库版本。

查找 OCCT 配置：

```powershell
Get-ChildItem C:\dev\occt793 -Recurse -Filter OpenCASCADEConfig.cmake
```

在 x64 Native Tools 命令提示符中，将以下示例路径改成真实路径：

```bat
cmake -S . -B build-sdk -G "Visual Studio 17 2022" -A x64 -DQt6_DIR="C:/Qt/6.8.4/msvc2022_64/lib/cmake/Qt6" -DOpenCASCADE_DIR="C:/dev/occt793/cmake"
cmake --build build-sdk --config Release --parallel
ctest --test-dir build-sdk -C Release --output-on-failure
```

运行时将 Qt bin、OCCT Release bin、第三方 DLL 目录加入 PATH。本仓库一键脚本针对 `.local` 依赖环境；手动 SDK 应自己设置路径，不要交叉使用。

Mac 已有 Homebrew 时也可使用 `brew install cmake ninja qt opencascade`，但必须检查 OCCT 仍是 7.9.x。然后把两个 brew prefix 传给 `CMAKE_PREFIX_PATH`。以后包管理器更新到 OCCT 8.x 时，需要明确迁移，不能仅删除版本检查。

## 4. Qt Creator

打开根目录 `CMakeLists.txt`，选本机编译器和 Qt Kit。若使用独立环境，把 `Qt6_DIR` 设为该环境中 Qt6Config.cmake 所在目录，`OpenCASCADE_DIR` 设为 OpenCASCADEConfig.cmake 所在目录。Windows 运行环境 PATH 要含 `.local\env\Library\bin`。

首次用 Release。Debug 需要依赖也有兼容的 Debug 运行库，不能只切换自己的编译选项。

## 5. 常见报错

| 报错 | 检查 |
|---|---|
| 找不到 Qt6Config | 是否装了 SDK；Qt6_DIR 是目录不是文件 |
| 找不到 OpenCASCADEConfig | 不要填 OCCT 源码根目录 |
| TKDESTEP 不存在 | OCCT 版本/分库方式/数据交换模块 |
| LNK2019 | 库是否链接，编译器与 x64 是否一致 |
| 辅助函数处出现不合理语法错误 | 检查 Windows 头文件宏；例如旧宏 near 会与同名函数冲突，本工程使用 expectNear |
| DLL 不存在 | 运行脚本设置的 PATH 是否生效 |
| wrong architecture | Mac arm64/x86_64 混用，清理相应 build 目录重建 |
| Qt platform plugin 错误 | 检查是否混用了全局 QT_PLUGIN_PATH 或另一份 Qt |
| 原生窗口黑屏 | 在终端启动查看日志，确认 OpenGL 与本机桌面可用 |

依赖下载失败时先检查网络和代理。不要在安装一半时把缺失的库随意从其他版本复制进来。
