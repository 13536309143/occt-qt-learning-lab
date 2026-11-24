# Windows 本机部署验证记录

日期：2026-11-24（Asia/Shanghai）。在 `E:\QT\occt-qt-learning-lab` 从远端提交 `445ffd7` 克隆，修复下述 Windows 脚本后验证。应用 C++ 源码未改动。

## 环境

| 项目 | 实际环境 |
|---|---|
| 系统 | Windows 11 Pro x64，10.0.26300；Qt 报告为 Version 26H2 |
| 编译器 | VS 2022 Professional 17.14.23，MSVC 19.44.35222，x64 |
| Shell | Windows PowerShell 5.1 |
| 依赖 | Qt 6.8.4、OCCT 7.9.3（novtk）、CMake 4.5.1、Ninja；Release |
| 显卡 | NVIDIA GeForce RTX 3060，驱动 32.0.16.1088；系统另有远程显示适配器 |
| 依赖管理 | 仓库 `.local` 下的 micromamba / conda-forge，65 个包，安装时约下载 229 MB |

精确依赖 URL 见 [environment-win-64.explicit.txt](environment-win-64.explicit.txt)，仅适用于 win-64。显卡信息是系统枚举结果，未单独采集 OpenGL 实际 renderer 字符串。

## 自动检查

| 内容 | 结果 | 原始记录 |
|---|---|---|
| CMake 配置、Release 构建 | 通过 | [configure.log](configure.log)、[build.log](build.log)（最后一次为增量构建） |
| 模型、非法输入、STEP/BREP/STL | 36/36 | [model-results.json](model-results.json) |
| 默认缩放真实窗口检查，DPR=1 | 18/18 | [gui-results.json](gui-results.json) |
| `QT_SCALE_FACTOR=1.5`，DPR=1.5 | 18/18 | [scaled-150/gui-results.json](scaled-150/gui-results.json) |
| `QT_SCALE_FACTOR=2`，DPR=2 | 18/18 | [scaled-200/gui-results.json](scaled-200/gui-results.json) |
| CTest 两个套件 | 2/2，约 8.34 秒 | [ctest.log](ctest.log)、[ctest.xml](ctest.xml)、[LastTest.log](LastTest.log) |

共执行 90 项自动检查；三组 GUI 是同一组 18 项实验在不同缩放配置下执行。默认缩放的 14 张 PNG 在 [docs/screenshots/windows](../../docs/screenshots/windows)，已检查代表性应用截图；它们来自真实窗口与 OCCT framebuffer。

## 桌面补验

使用运行脚本打开程序，通过 Windows 原生文件对话框保存 `build-win\中文验证.step`，文件实际生成（15429 字节），再次打开成功。Info 显示 6 个面、12 条边、8 个顶点、1 个实体，体积 240000.000 mm³，与默认 100×80×30 长方体一致。再次打开对话框并取消，模型统计保持；Alt+F4 后窗口退出，再用运行脚本启动成功。

这部分通过 computer-use 操作真实窗口，不计入自动检查数量。程序最后保持打开，便于继续体验。

## 本次修复

1. VS 安装路径含空格时，原 `cmd /s /c` 引号在 PowerShell 5.1 下解析失败，提示 `D:\Program` 不是命令。改为 `cmd /d /c` 配合 `call` 初始化 `vcvars64.bat`。
2. `$ErrorActionPreference = 'Stop'` 将原生命令 stderr 的 CMake 弃用警告当作终止错误。构建脚本局部保留 stderr 日志，以进程退出码判定失败，非零退出仍抛错。
3. conda-forge Qt 的插件在 `Library\lib\qt6\plugins`。原脚本仅设置 PATH，应用弹出缺少平台插件的错误框，CTest 等待 180 秒后超时。构建和运行脚本现在明确设置 `QT_PLUGIN_PATH` 与 `QT_QPA_PLATFORM_PLUGIN_PATH`，随后全部窗口实验通过。
4. 启动脚本在 EXE 不存在时提示先安装和构建。

CMake 4.5.1 与 Qt 6.8.4 组合仍有 Qt 设置旧 CMP0156 策略的弃用警告，未影响配置、编译或测试，原文保留于配置日志。

## 复跑

在仓库根目录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run-windows.ps1
```

强制缩放实验，在另一个 PowerShell 中执行：

```powershell
$prefix = Join-Path (Get-Location) '.local\env'
$env:Path = "$prefix\Library\bin;$prefix;$env:Path"
$env:QT_PLUGIN_PATH = "$prefix\Library\lib\qt6\plugins"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$env:QT_PLUGIN_PATH\platforms"
$env:QT_SCALE_FACTOR = '1.5'
& .\build-win\OcctQtStarter.exe --self-test --output build-win\gui-scale150
# 检查 $LASTEXITCODE 为 0，再运行另一组
$env:QT_SCALE_FACTOR = '2'
& .\build-win\OcctQtStarter.exe --self-test --output build-win\gui-scale200
Remove-Item Env:QT_SCALE_FACTOR
```

未验证不同物理 DPI 显示器之间移动、所有显卡驱动、物理鼠标手感、文件覆盖确认、受限目录写入对话框、长期稳定性及不带开发环境的干净机器分发。本记录证明当前 Windows 本机的源码部署、建模和桌面工作流通过，详细人工验收表保留于 [测试教程](../../docs/05-testing.md)。
