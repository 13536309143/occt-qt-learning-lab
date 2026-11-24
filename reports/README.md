# 测试证据说明

本机 Mac 的最终测试记录位于 `macos-2026-11-24/`。

| 文件 | 含义 |
|---|---|
| model-results.json | 36 项模型和文件交换检查 |
| gui-results.json | 18 项普通缩放下的窗口实验 |
| scaled-gui/gui-results.json | 同一 18 项实验，QT_SCALE_FACTOR=2 |
| LastTest.log | CTest 原始输出，包含预期失败输入的诊断 |
| environment-osx-arm64.explicit.txt | 本次 Mac 依赖包的精确下载地址 |
| gui-scale2.log | 强制缩放实验日志，保留系统和 Qt 警告 |

本机共执行 36+18+18=72 项检查，全部通过；其中后两组是相同实验在不同缩放条件下执行。课程图片在 `docs/screenshots/`；缩放实验截图保留在 `scaled-gui/`。

`configure-mac.log`、`build-mac.log`、`ctest-mac.log` 与 `ctest-mac.xml` 为最终构建和 CTest 记录。模型数值测试有独立的数学期望；图像测试验证可解码、尺寸和非空像素，同时人工检查代表截图。

GitHub Actions 的 Windows/Mac 运行与 artifact 是独立证据，不能替代本机真实桌面测试。Windows 本机脚本将结果放到 `windows-local/` 和 `build-win/*-artifacts`，默认不提交本机日志。

截图 01—12 与 14 是 OCCT 实际视口导出；13 是系统抓取应用窗口区域。它们没有使用 AI 图像生成或重新绘制。
