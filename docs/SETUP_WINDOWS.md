# Windows 零基础环境配置

这份文档面向从未配置过 ESP32 / ESP-IDF 环境的人。按顺序做完后，可以编译并烧录本项目的触摸屏虚拟手柄固件。

## 1. 需要准备的硬件

- WT9932P4-TINY / ESP32-P4 开发板
- 7 英寸 1024 x 600 MIPI-DSI 触摸屏，屏幕驱动为 EK79007，触摸芯片为 GT911
- USB-C 数据线，必须支持数据传输，不能只支持充电
- Windows 10 / Windows 11 电脑

## 2. 下载软件

请先下载并安装下面这些软件。

| 软件 | 下载地址 | 用途 |
| --- | --- | --- |
| Git for Windows | https://git-scm.com/download/win | 克隆 GitHub 项目 |
| Visual Studio Code | https://code.visualstudio.com/Download | 打开和编辑工程 |
| ESP-IDF Windows Installer | https://dl.espressif.com/dl/esp-idf/ | 安装 ESP-IDF 编译环境 |
| USB 串口驱动 | https://www.wch.cn/downloads/CH343SER_EXE.html | 如果电脑识别不出串口再安装 |

ESP-IDF 推荐版本：`v5.4.x`。

安装 ESP-IDF 时建议选择：

- `ESP-IDF v5.4.x`
- 安装路径可用默认路径，也可以用 `E:\Vscode_codes\esp-idf-v5.4`
- 目标芯片需要包含 `esp32p4`

## 3. 克隆项目

打开 Windows 的 `cmd` 或 PowerShell，执行：

```bat
git clone https://github.com/snsn78/WT9932P4-TINY.git
cd WT9932P4-TINY\lvgl_demo_v9
```

如果不会打开命令行：

1. 按 `Win + R`
2. 输入 `cmd`
3. 回车
4. 再复制上面的命令执行

## 4. 连接硬件

屏幕连接方式见 [HARDWARE.md](HARDWARE.md)。

重点检查：

- MIPI-DSI 排线方向正确
- 屏幕转接板供电已接好
- GT911 触摸线已连接
- 开发板电源开关已打开
- USB-C 连接到电脑后，设备管理器里能看到 COM 串口

## 5. 激活 ESP-IDF 环境

如果你使用 ESP-IDF Windows Installer 安装，开始菜单里会出现类似下面的入口：

```text
ESP-IDF 5.4 CMD
```

打开它，然后进入项目：

```bat
cd /d <你的项目路径>\WT9932P4-TINY\lvgl_demo_v9
```

示例：

```bat
cd /d D:\code\WT9932P4-TINY\lvgl_demo_v9
```

## 6. 编译固件

在 `lvgl_demo_v9` 目录中执行：

```bat
idf.py set-target esp32p4
idf.py build
```

第一次编译会自动下载 ESP-IDF 组件，时间会比较长。成功时会看到类似：

```text
Project build complete.
```

## 7. 查找串口号

打开 Windows 设备管理器：

```text
设备管理器 -> 端口 (COM 和 LPT)
```

找到开发板对应的串口，例如：

```text
USB-SERIAL CH343 (COM20)
```

记下 `COM20`，你的电脑上可能是 `COM3`、`COM5`、`COM12` 等。

## 8. 烧录固件

把下面命令里的 `COM20` 换成你的串口号：

```bat
idf.py -p COM20 flash
```

也可以使用项目自带脚本：

```bat
flash_com20.bat COM20
```

脚本会自动：

1. 检查 ESP-IDF 环境
2. 编译项目
3. 检查串口
4. 烧录固件
5. 询问是否打开串口监视器

## 9. 查看运行日志

```bat
idf.py -p COM20 monitor
```

退出监视器：按 `Ctrl + ]`。

正常启动时，应看到类似日志：

```text
GamePad firmware booting
Display started
Backlight on
Creating GamePad UI
GamePad UI created
HID transport boundary started
```

## 10. 常见问题

### 找不到 idf.py

说明没有打开 ESP-IDF 专用终端。请从开始菜单打开：

```text
ESP-IDF 5.4 CMD
```

不要用普通 cmd 直接运行。

### 找不到 COM 口

按顺序检查：

1. 换一根支持数据传输的 USB-C 线
2. 换电脑 USB 口
3. 安装 CH343/CH34x 串口驱动
4. 重新插拔开发板
5. 打开设备管理器确认 COM 号

### 烧录失败，提示连接不上

可以尝试手动进下载模式：

1. 按住开发板 `BOOT`
2. 点按一下 `RESET`
3. 松开 `RESET`
4. 松开 `BOOT`
5. 重新执行烧录命令

### 屏幕亮了但没有 UI

通常是屏幕排线、供电、背光、屏幕型号或 BSP 配置问题。先确认：

- 屏幕是 EK79007 1024 x 600
- MIPI-DSI 排线方向正确
- 屏幕转接板 5V/GND/PWM/LCD_RST 已接好
- 串口日志里是否出现 `Display started` 和 `GamePad UI created`

### 能显示 UI，但触摸没有反应

优先检查 GT911 触摸连接：

- 触摸排线方向
- I2C 线是否接好
- 触摸芯片是否为 GT911
- 串口日志里是否出现 GT911 初始化失败信息

## 11. 项目目录说明

```text
WT9932P4-TINY/
├── lvgl_demo_v9/          # 触摸屏手柄主固件工程
├── src/                   # 虚拟手柄 UI、触摸输入、HID 映射代码
├── tests/                 # PC 侧逻辑测试
├── docs/                  # 新手教程和硬件说明
├── blink/                 # 原厂 LED 示例
├── lvgl_demo_v8/          # 原厂 LVGL v8 示例
└── video_lcd_display/     # 原厂视频显示示例
```

正常使用只需要关注：

```text
lvgl_demo_v9/
src/
docs/
```
