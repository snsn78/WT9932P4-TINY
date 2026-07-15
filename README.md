# ESP32-P4 触摸屏虚拟手柄

这是一个基于 **WT9932P4-TINY / ESP32-P4 + LVGL 9 + GT911 触摸屏** 的触摸屏虚拟手柄项目。

项目目标是：在硬件连接正确的情况下，用户 clone 本仓库后，按教程安装 ESP-IDF 环境、编译、烧录，即可在屏幕上运行触摸屏手柄界面。

## 当前功能

- 7 英寸 1024 x 600 MIPI-DSI 屏幕显示 LVGL 虚拟手柄 UI
- GT911 触摸输入
- 多点触控输入桥接，支持多个虚拟按键/摇杆同时响应
- 虚拟手柄输入状态层
- 游戏手柄 HID 报告映射
- USB HID 设备边界层
- PC 侧逻辑测试代码

## 适用硬件

- WT9932P4-TINY / ESP32-P4 开发板
- 7 英寸 1024 x 600 MIPI-DSI LCD 屏幕
- EK79007 LCD 驱动芯片
- GT911 触摸芯片
- Windows 10 / Windows 11 电脑

硬件连接见：[docs/HARDWARE.md](docs/HARDWARE.md)

## 新手从零开始

如果你从来没有配置过 ESP32 环境，请直接看：

[docs/SETUP_WINDOWS.md](docs/SETUP_WINDOWS.md)

里面包含：

- Git 下载
- VS Code 下载
- ESP-IDF 下载和安装
- 串口驱动下载
- 克隆项目
- 编译固件
- 查找 COM 口
- 烧录固件
- 查看串口日志
- 常见问题排查

## 给本地大模型 / AI 编程助手

如果你想把这个仓库直接交给本地大模型、Claude Code、Cursor、Copilot 或其他 AI 编程助手，请让它优先读取：

- [AGENTS.md](AGENTS.md) — 通用 AI 接手规则、项目边界、构建命令
- [CLAUDE.md](CLAUDE.md) — Claude Code 专用上下文
- [.cursorrules](.cursorrules) — Cursor / 类 Cursor 工具上下文
- [.github/copilot-instructions.md](.github/copilot-instructions.md) — GitHub Copilot 专用上下文
- [docs/PROJECT_MAP.md](docs/PROJECT_MAP.md) — 项目结构、关键模块、风险区域
- [docs/VALIDATION.md](docs/VALIDATION.md) — 编译、烧录、运行验收标准
- [docs/LOCAL_AI_HANDOFF_PROMPT.md](docs/LOCAL_AI_HANDOFF_PROMPT.md) — 可直接复制给本地大模型的交接提示词

这些文件的目的：让对方的本地 AI 不用猜项目入口、不误删本地 BSP/LVGL 补丁、不把构建产物传上 GitHub，并且知道如何验证固件真的能编译运行。

## 快速使用

已经安装好 ESP-IDF 5.4 的用户，可以直接执行：

```bat
git clone https://github.com/snsn78/WT9932P4-TINY.git
cd WT9932P4-TINY\lvgl_demo_v9
idf.py set-target esp32p4
idf.py build
idf.py -p COM20 flash monitor
```

把 `COM20` 换成你电脑上的实际串口号。

也可以使用脚本：

```bat
flash_com20.bat COM20
```

## 主要目录

```text
WT9932P4-TINY/
├── lvgl_demo_v9/          # 触摸屏虚拟手柄主固件工程
├── src/                   # 手柄 UI、触摸输入、HID 映射等核心代码
├── tests/                 # PC 侧逻辑测试
├── docs/                  # 新手环境配置和硬件连接文档
├── blink/                 # 原厂 LED 示例
├── lvgl_demo_v8/          # 原厂 LVGL v8 示例
└── video_lcd_display/     # 原厂视频显示示例
```

正常使用本项目时，主要关注：

```text
lvgl_demo_v9/
src/
docs/
```

## 运行成功标志

烧录后打开串口监视器：

```bat
idf.py -p COM20 monitor
```

正常日志中应看到：

```text
GamePad firmware booting
Display started
Backlight on
Creating GamePad UI
GamePad UI created
HID transport boundary started
```

屏幕上应显示虚拟手柄界面，并且触摸控件可以响应。

## 原厂示例说明

本仓库基于 WT9932P4-TINY 开发板示例工程整理，仍保留原厂示例：

1. `blink`: 板载 RGB 灯闪烁示例
2. `lvgl_demo_v8`: 基于 LVGL v8 的 MIPI LCD 驱动示例
3. `lvgl_demo_v9`: 基于 LVGL v9 的 MIPI LCD 驱动，本项目的触摸屏手柄主工程
4. `video_lcd_display`: MIPI CSI 摄像头显示到 MIPI LCD 示例

