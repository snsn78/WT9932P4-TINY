# 摄像头显示例程

该示例演示了如何移植 LVGL v9 并使用 LVGL 内置的示例进行性能测试。该示例使用了开发板的 MIPI-DSI 接口。基于此示例，可以开发基于 LVGL v9 的应用。

## 快速入门

### 准备工作

* 一块 WT9932P4-TINY 开发板。
* 一块由 [EK79007](https://dl.espressif.com/dl/schematics/display_driver_chip_EK79007AD_datasheet.pdf) 芯片驱动的 7 英寸 1024 x 600 LCD 屏幕，([LCD 规格](https://dl.espressif.com/dl/schematics/display_datasheet.pdf))。
* 用于供电和编程的 USB-C 电缆。
* 请参考以下步骤进行连接：
    * **步骤 1**. 根据下表，将屏幕适配板背面的引脚连接到开发板的相应引脚。

        | 屏幕适配板            | ESP32-P4-Function-EV-Board |
        | -------------------- | -------------------------- |
        | 5V（任意一个）        | 5V（任意一个）              |
        | GND（任意一个）       | GND（任意一个）             |
        | PWM                  | GPIO26                     |
        | LCD_RST              | GPIO27                     |
        | INIT_TP              | GND（任意一个）             |


    * **步骤 2**. 通过 `MIPI_DSI` 接口连接 LCD 的 FPC (请注意线序，使用两面同面触点的FPC连接)。
    * **步骤 3**. 通过 `MIPI_CSI` 接口连接摄像头的 FPC (请注意线序，使用两面异面触点的FPC连接)。
    * **步骤 4**. 使用 USB-C 电缆将 `FUSB` 端口连接到 PC（用于供电和查看串行输出）。
    * **步骤 5**. 打开开发板的电源开关。

### ESP-IDF 要求

- 此示例支持 ESP-IDF release/v5.4 及以上版本。默认情况下，在 ESP-IDF release/v5.4 上运行。
- 请参照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 设置开发环境。**强烈推荐** 通过 [编译第一个工程](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#id8) 来熟悉 ESP-IDF，并确保环境设置正确。

### 配置


运行 ``idf.py menuconfig`` 并修改 ``Board Support Package(ESP32-P4)`` 配置：

```
menuconfig > Component config > Board Support Package
```

## 如何使用示例


### 编译和烧录示例

编译项目并将其烧录到开发板上，运行监视工具可查看串行端口输出（将 `PORT` 替换为所用开发板的串行端口名）：

```c
idf.py -p PORT flash monitor
```

输入``Ctrl-]`` 可退出串口监视。

有关配置和使用 ESP-IDF 来编译项目的完整步骤，请参阅 [ESP-IDF 快速入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 。
