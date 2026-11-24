# 05：测试、证据和 Windows 验收

## 1. 本次实际执行结果

执行日期：2026-11-24（Asia/Shanghai）。环境：Mac arm64、macOS 27.2.1、Apple Clang 21、Qt 6.8.4、OCCT 7.9.3、CMake 4.5.1，Release。

| 执行内容 | 实际结果 | 证据 |
|---|---|---|
| CMake 配置与编译 | 通过 | reports/configure-mac.log、build-mac.log |
| 独立模型检查 | 36/36 | macos-2026-11-24/model-results.json |
| 原生窗口检查 | 18/18 | macos-2026-11-24/gui-results.json |
| 强制 QT_SCALE_FACTOR=2 的窗口检查 | 18/18 | macos-2026-11-24/scaled-gui/gui-results.json |
| 截图 | 基准 14 张 + 缩放实验 14 张 | docs/screenshots、reports/.../scaled-gui |

CTest 把两个可执行测试套件分别计为 modeling 和 gui_experiments，因此会显示 2/2 通过；套件内部细分为 36+18 项。额外 scale2 是独立执行的同一 GUI 套件，共执行 72 项检查，不应说成 70 种不同测试。

强制缩放说明坐标路径在该配置下工作，不等同于验证所有真实 Retina/Windows 高 DPI、多屏切换和显卡组合。

## 2. 如何复跑

Mac 完整检查：

```bash
bash scripts/build-macos.sh
```

只运行模型：

```bash
.local/env/bin/ctest --test-dir build-mac -R '^modeling$' --output-on-failure
```

单独运行窗口与截图：

```bash
./build-mac/OcctQtStarter.app/Contents/MacOS/OcctQtStarter --self-test --output build-mac/gui-manual
QT_SCALE_FACTOR=2 ./build-mac/OcctQtStarter.app/Contents/MacOS/OcctQtStarter --self-test --output build-mac/gui-scale2
```

失败时检查退出码和 JSON 中 failed，不要只看截图目录是否存在。CTest 的 `LastTest.log` 保存每个套件内部输出，包括故意损坏文件触发的预期 OCCT 错误日志。

## 3. 建模测试覆盖

| 领域 | 验证方式 |
|---|---|
| 长方体 | 数学体积、重心、唯一拓扑数量、自定义尺寸 |
| 球 | 解析体积公式 |
| 带孔板 | 解析减法体积、实体数、面数 |
| 布尔 | 两个重叠立方体的并集/交集数学体积 |
| 拉伸 | 独立计算截面积×高度 |
| 回转 | 圆环柱体积公式 |
| 放样 | 相似截面棱台体积公式 |
| 圆角 | 合法性、实体数、去除材料 |
| 变换 | 体积不变、重心偏移 |
| 法兰 | 四孔与六孔体积公式 |
| 非法参数 | 零/负/NaN/无穷尺寸、孔越界/重叠、空形状 |
| STEP | 往返体积/实体、中文路径、缺失/损坏文件、写失败 |
| BREP | 往返体积、缺失文件 |
| STL | 非零三角形数、二进制文件结构 |

GUI 覆盖：初始化、实际工具栏按钮点击、面/边点击拾取、滚轮 Scale、旋转 Direction、平移 Center、Resize+Fit、各模型渲染、STEP GUI 方法往返、错误模型保留旧状态、Resize 后拾取。

## 4. Windows 本机必须执行的步骤

克隆后用 setup-windows / build-windows / run-windows 三个脚本。构建脚本默认启用真实窗口实验，应在登录桌面而非无显示 SSH 会话执行。

结果位置：

```text
reports/windows-local/configure.log
reports/windows-local/build.log
reports/windows-local/ctest.log
reports/windows-local/ctest.xml
build-win/test-artifacts/model-results.json
build-win/gui-artifacts/gui-results.json
build-win/gui-artifacts/*.png
```

`windows-local` 默认不提交，避免你本机路径和临时日志自动公开。需要分享时先检查内容，再按需上传。

人工验收表：

- [ ] 打开程序，Box / Plate / 所有实验能正常显示。
- [ ] Windows 缩放 100%、150%、200% 下分别检查拾取位置。
- [ ] 在不同缩放比例的显示器之间移动窗口，再检查 Face / Edge。
- [ ] 使用物理鼠标旋转、平移、滚轮，Fit 后模型完整。
- [ ] Shift+左键多选、空白处清空选择符合预期。
- [ ] Parameters 将孔数改成 6，查看体积；不合法输入保留原模型。
- [ ] 文件对话框的取消、覆盖、中文目录、无写权限位置。
- [ ] 导出 STEP 后重开，尺寸与体积一致。
- [ ] 关闭、重新启动，观察有无崩溃或残留窗口。

GitHub Actions 自动执行 Windows / Mac 的编译和模型检查。它不会宣称验证物理显示器和显卡，也不在无桌面 runner 上执行 OpenGL 窗口实验。

## 5. 明确未覆盖的范围

没有穷举随机复杂 BRep、所有布尔退化情况、巨大装配、模型修复、STEP 装配颜色/PMI、长期内存增长、并发修改、GPU 驱动差异、休眠恢复、签名/公证与干净机器部署。需要产品质量时，应基于你的实际样本继续增加回归集。

本仓库不能因为有限测试通过，就断言某个 OCCT 算法对任意模型都可靠。
