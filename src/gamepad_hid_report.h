#ifndef GAMEPAD_HID_REPORT_H
#define GAMEPAD_HID_REPORT_H

#include <stdint.h>

#include "gamepad_output.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HID report 适配层 (Step 12，预留接口)
 * ------------------------------------------------------------------
 * 把与 UI 解耦的输出报文(gamepad_output_report_t)转换为一个类标准手柄 HID 报文
 * 的字节布局。当前仅提供数据打包，不含 USB/BLE 传输 —— 传输栈接入时直接复用本结构。
 *
 * 轴范围采用有符号 8 位 [-127,127]，符合常见 HID 摇杆约定；
 * 注意：HID 习惯 Y 轴向下为正，本函数会把状态层的“上为+”翻转为“下为+”。
 */

typedef struct __attribute__((packed)) {
    int8_t left_x;    /* 左摇杆 X, [-127,127] 右为 + */
    int8_t left_y;    /* 左摇杆 Y, [-127,127] 下为 + (HID 习惯) */
    int8_t right_x;   /* 右摇杆(视角) X */
    int8_t right_y;   /* 右摇杆(视角) Y, 下为 + */
    uint16_t buttons; /* 位布局同 gamepad_output_report_t.buttons */
    uint8_t hat;      /* 8 方向 hat：0..7 顺时针(0=上)，0x0F=居中 */
} gamepad_hid_report_t;

#ifdef __cplusplus
static_assert(sizeof(gamepad_hid_report_t) == 7,
              "gamepad_hid_report_t must match the 7-byte USB HID report");
#else
_Static_assert(sizeof(gamepad_hid_report_t) == 7,
               "gamepad_hid_report_t must match the 7-byte USB HID report");
#endif

#define GAMEPAD_HID_HAT_CENTER 0x0F

/* 由输出报文打包 HID 报文。out 为空则不动作。 */
void gamepad_hid_report_build(const gamepad_output_report_t * out, gamepad_hid_report_t * hid);

#ifdef __cplusplus
}
#endif

#endif /* GAMEPAD_HID_REPORT_H */
