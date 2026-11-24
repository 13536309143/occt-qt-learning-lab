# 从零学习 Qt + Open CASCADE：Mac / Windows 完整实验教程

用同一套 C++17 源码学习三维窗口、几何拓扑、建模、选择和 CAD 文件交换。基线版本为 **Qt 6.8.4 + OCCT 7.9.3**。每个实验都有源码、运行方法和验收条件。

![Mac 实际程序截图](docs/screenshots/13-application-screen.png)

上图来自本机真实窗口。其余截图是 OCCT 从实际 OpenGL 帧缓冲导出的视口图片，未使用生成式图像。

## 从这里开始

| 顺序 | 教程 | 完成后能做什么 |
|---|---|---|
| 1 | [双平台安装与运行](docs/01-setup.md) | 编译工程并运行测试 |
| 2 | [基础概念与代码阅读](docs/02-foundations.md) | 分清几何、拓扑、显示对象 |
| 3 | [建模实验 01—10](docs/03-modeling-labs.md) | 基本体、布尔、拉伸、旋转、放样、圆角、法兰 |
| 4 | [交互与文件实验 11—18](docs/04-viewer-and-exchange.md) | 拾取、相机、STEP/BREP/STL |
| 5 | [测试覆盖、证据和 Windows 验收](docs/05-testing.md) | 判断实验是否真正通过 |
| 6 | [独立项目、架构与发布](docs/06-next-project.md) | 扩展参数化建模工具 |

## 获取完整工程

```bash
git clone https://github.com/13536309143/occt-qt-learning-lab.git
cd occt-qt-learning-lab
```

## Mac 快速开始

先安装 Xcode Command Line Tools：`xcode-select --install`。进入仓库后执行：

```bash
bash scripts/setup-macos.sh
bash scripts/build-macos.sh
open build-mac/OcctQtStarter.app
```

依赖安装在仓库 `.local/` 下，不要求已有 Homebrew，不修改全局 Qt 环境。首次下载需要联网和数百 MB 空间。`.local/` 与 `build-*` 不提交 Git。

## Windows 快速开始

先安装 **Visual Studio 2022 → 使用 C++ 的桌面开发**，然后在 Windows PowerShell 中进入仓库：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\run-windows.ps1
```

`ExecutionPolicy Bypass` 仅针对启动的这一个 PowerShell 进程。脚本使用 x64 MSVC，依赖同样安装在 `.local/`。详细路径、手动安装与报错排查见安装章节。

## 当前可复核的本机结果

- Mac arm64、OCCT 7.9.3、Qt 6.8.4：已实际编译并运行。
- 36 项建模、数学属性、非法输入和文件交换检查：全部通过。
- 17 项原生窗口、鼠标事件、拾取、渲染和文件交互检查：全部通过。
- [原始建模结果](reports/macos-2026-11-24/model-results.json)、[原始 GUI 结果](reports/macos-2026-11-24/gui-results.json)、[CTest 记录](reports/macos-2026-11-24/LastTest.log)。
- 另执行 QT_SCALE_FACTOR=2 的同一组窗口实验，17/17 通过；[原始记录](reports/macos-2026-11-24/scaled-gui/gui-results.json)。
- Windows 结果以 [Actions](https://github.com/13536309143/occt-qt-learning-lab/actions) 的实际运行和你本机测试为准。没有执行的物理高分屏、显卡和文件对话框检查不会记作通过。

## 界面操作

| 控件 / 输入 | 功能 |
|---|---|
| Box / Plate + hole | 长方体 / 带孔板 |
| More experiments | L 形拉伸、回转体、放样、圆角、法兰 |
| Parameters | 输入法兰参数并重建 |
| Fit | 适应视口 |
| Object / Face / Edge | 切换整件、面、边选择 |
| Info | 唯一拓扑数量与体积 |
| Open STEP / Save STEP | 替换当前模型 / 导出当前模型 |
| 左键 / Shift+左键 | 选择 / 切换多选 |
| 右键拖动 / 中键拖动 / 滚轮 | 旋转 / 平移 / 缩放 |

本教程约定 1 个建模单位 = 1 mm，导入导出也按毫米设置。当前应用是单模型教学工具；导入会替换当前模型。普通 STEP 形状接口不保存应用特征树、装配元数据和显示颜色。STL 是三角网格，不能等同于原始精确 BRep。

## 源码地图

```text
main.cpp                 Qt 工具栏、参数对话框、命令行入口
cad_view.h               视图生命周期、显示与鼠标事件
native_window.cpp        WNT_Window / Cocoa_Window 平台适配
modeling.h / .cpp        无 Qt 依赖的建模与文件交换
tests/model_tests.cpp    36 项可独立运行的模型检查
gui_experiments.cpp      17 项真实窗口实验与截图输出
scripts/                 Mac / Windows 安装、构建和运行
docs/                    中文课程与截图
samples/                 实际实验导出的 STEP、BREP、STL
reports/                 原始测试证据与环境锁定信息
.github/workflows/       Mac / Windows 自动编译与建模检查
```

测试代码使用 Qt Test 模拟事件；截图和几何数值来自实际 OCCT 执行结果。课程覆盖的是完整入门工作流，不宣称穷尽 OCCT 全部算法或全部输入情况。

## 官方资料和版本

- [OCCT 7.9.3 源码和离线文档](https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V7_9_3)：推荐使用同版 overview-doc 和 refman-doc。
- [OCCT 官方文档入口](https://occt3d.com/dev/doc/overview/html/index.html)：在线版本可能比教程新。
- [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html)。
- [Qt / OCCT 集成示例](https://github.com/gkv311/occt-samples-qt)：示例自述验证版本为 7.7，不应直接视作 8.x 项目。
- [8.x 升级说明](https://occt3d.com/dev/doc/overview/html/occt__upgrade.html)。

本仓库自编代码和课程使用 MIT；Qt、OCCT 和其他依赖遵循各自许可证，安装脚本不会改变这些依赖的授权条款。
