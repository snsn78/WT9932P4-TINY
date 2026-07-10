# ESP32-P4 LVGL虚拟手柄整合路线与详细任务清单

## 文档定位

这份文档只服务于当前已经确认的正确方向：

- 目标平台是 `ESP32-P4`
- 宿主工程是 `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9`
- UI来源主要在 `E:\Vscode_codes\lvgl\src`
- 最终目标不是“做一个普通LVGL页面”，而是“做一套可编辑、可保存、可映射、可输出的LVGL虚拟手柄系统”

这次不再沿用 STM32 那条路线，也不再把 USB Host 键盘/鼠标方案当成当前主线。

---

## 当前已经确认的工程事实

### 1. 宿主工程入口

- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\main.c`
- 当前入口函数是 `app_main()`
- 当前已经调用：
  - `xbox_touch_gamepad_layout_demo_create()`

### 2. 当前编译接入方式

- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`
- 当前通过 `../../../src/*.c` 的方式把共享UI代码编译进ESP32-P4工程

### 3. 当前已经接入的主要模块

- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.h`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.c`
- `E:\Vscode_codes\lvgl\src\gamepad_button_module.c`
- `E:\Vscode_codes\lvgl\src\joystick_stick_demo.c`
- `E:\Vscode_codes\lvgl\src\Dpad.c`
- `E:\Vscode_codes\lvgl\src\shoulder_keys_demo.c`
- `E:\Vscode_codes\lvgl\src\codex_gpt5_slider_2d_demo.c`
- `E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_module_builders.h`

### 4. 当前真实状态

当前已经有：

- 可显示的 Xbox Touch Layout 页面
- 可切换编辑模式和播放模式
- 可拖拽、缩放模块
- 可保存/加载 slot 布局
- 左摇杆、右侧视角滑块、方向键、肩键、A/B/X/Y按钮等模块

当前还没有真正完成：

- 统一输入状态层
- 控件事件到统一状态的收口
- 布局配置里的动作绑定字段
- 运行时映射层
- 编辑器里的“绑定编辑”
- 最终输出层

---

## 当前建议的默认映射清单

这份映射清单是基于你最新确认的意图整理的，也就是：

- 左摇杆 = 普通手柄移动
- 右侧滑块/触控区 = 视角转动
- 其余尽量按 Xbox 语义布局

| UI模块/预设 | 当前来源 | 默认动作语义 | 输出字段建议 |
|---|---|---|---|
| `LEFT_STICK` | 左摇杆模块 | 角色移动 | `move_x`, `move_y` |
| `VIEW_SLIDER` | 右侧2D滑块 | 视角转动 | `look_x`, `look_y` |
| `DPAD` | 十字方向键 | 方向/菜单/快捷输入 | `dpad_up`, `dpad_down`, `dpad_left`, `dpad_right` |
| `A` | 单按钮 | 确认/跳跃/主动作 | `btn_a` |
| `B` | 单按钮 | 返回/取消/副动作 | `btn_b` |
| `X` | 单按钮 | 交互/功能1 | `btn_x` |
| `Y` | 单按钮 | 切换/功能2 | `btn_y` |
| `L1` | 左肩上键 | 左肩键 | `btn_l1` |
| `L2` | 左扳机 | 左扳机键 | `btn_l2` |
| `R1` | 右肩上键 | 右肩键 | `btn_r1` |
| `R2` | 右扳机 | 右扳机键 | `btn_r2` |
| `START` | 单按钮扩展 | 菜单/暂停 | `btn_start` |
| `SELECT` | 单按钮扩展 | 地图/背包/返回页 | `btn_select` |

---

## 最终你会得到什么效果

最终完成后，你应该得到的是一套运行在 `ESP32-P4 + LVGL9` 上的虚拟手柄系统：

1. 屏幕上能看到完整的可视化手柄界面。
2. 左摇杆拖动时，会持续输出移动向量。
3. 右侧2D滑块拖动时，会持续输出视角向量。
4. A/B/X/Y、L1/L2、R1/R2、DPad 都能进入统一状态层。
5. 编辑模式下可以拖动、缩放、增删模块。
6. 编辑模式下还能修改每个模块背后的动作绑定。
7. 配置可保存到 slot，重启后还能恢复。
8. 后续可以把统一状态导出给：
   - 本地业务逻辑
   - 虚拟手柄协议层
   - HID report 适配层

---

## 如何进行可视化查看

你后续主要通过这三种方式看结果：

### 1. 直接看ESP32-P4屏幕界面

启动工程后，屏幕上会显示：

- 顶部工具栏
- 中央手柄布局区域
- 左摇杆
- 右侧视角滑块
- 方向键
- 肩键
- 编辑面板和 slot 面板

### 2. 看调试状态层

后续建议增加一个屏幕调试栏，实时显示：

- `LX`, `LY`
- `RX`, `RY`
- `A/B/X/Y`
- `L1/L2/R1/R2`
- `DPad`
- 当前选中的模块绑定

### 3. 看日志输出

在调试阶段，允许保留少量串口日志，例如：

- `move_x=... move_y=...`
- `look_x=... look_y=...`
- `btn_a=1`

但日志只能用于辅助验证，不能继续作为正式逻辑通道。

---

## 整体实施原则

### 原则1：先把状态流打通，再谈协议输出

当前最缺的不是页面，而是：

- 输入控件
- 统一状态
- 动作绑定
- 输出接口

所以先把状态流打通，后续无论接业务还是接 HID，都不会跑偏。

### 原则2：编辑器改的是配置，不是代码逻辑

编辑模式应该修改：

- 布局位置
- 尺寸
- 可见性
- 动作绑定
- 参数

而不是直接去改某个控件的硬编码逻辑。

### 原则3：当前阶段先不急着大规模搬文件

因为现在 `main/CMakeLists.txt` 已经稳定引用了 `E:\Vscode_codes\lvgl\src` 下的文件，所以短期内继续在这里演进最稳。

---

# 详细任务拆分

## Step 1：锁定唯一宿主工程与入口边界

### 角色指定

你现在是“整合负责人”，这一步的职责不是写功能，而是先把项目边界定死，避免再次混到 STM32 路线里。

### 项目背景信息

之前的讨论里已经出现过“目标平台混乱”的问题。现在已经确认真正要落地的是 `ESP32-P4`，所以第一步必须明确：

- 哪个工程是宿主
- 哪个函数是入口
- 哪些源码已经被编译进来

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\main.c`
- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`
- `E:\Vscode_codes\lvgl\src`

### 本步骤的任务

1. 确认当前唯一宿主工程为 `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9`
2. 确认程序入口函数为 `app_main()`
3. 确认 `app_main()` 当前真正启动的是 `xbox_touch_gamepad_layout_demo_create()`
4. 确认 `main/CMakeLists.txt` 当前已经编译接入了哪些 `src/*.c`
5. 把 STM32 路线从当前主计划中正式剥离，不再作为当前交付目标

### 所能够调用的skill或者其他辅助项目的工具

- 工程文件检索
- 文本搜索
- CMake文件核对
- 入口函数核对

### 不做什么事情

- 不新增任何功能代码
- 不修改 UI 控件行为
- 不修改布局模型

### 查验标准

- 团队内对“当前宿主工程是谁”没有歧义
- 团队内对“最终改动应该落在哪个工程”没有歧义
- 当前文档后续所有步骤都只围绕 ESP32-P4 工程展开

---

## Step 2：盘点当前模块现状，确认哪些还是 demo 输出

### 角色指定

你现在是“现状审计者”，这一步的职责是判断现有模块哪些已经够用，哪些只是 demo。

### 项目背景信息

虽然 UI 已经跑起来了，但不少控件还是“看起来像手柄”，本质上还只是打印日志的演示模块。后续所有重构都必须建立在这个盘点结果上。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\joystick_stick_demo.c`
- `E:\Vscode_codes\lvgl\src\codex_gpt5_slider_2d_demo.c`
- `E:\Vscode_codes\lvgl\src\Dpad.c`
- `E:\Vscode_codes\lvgl\src\shoulder_keys_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_button_module.c`
- `E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`

### 本步骤的任务

1. 逐个核对每个模块是否还在使用 `printf`
2. 核对每个模块的触摸事件入口
3. 标记哪些模块已经具备“控件视觉外形”
4. 标记哪些模块还缺“正式状态输出”
5. 形成一份明确结论：
   - 哪些模块后续只需要改事件输出
   - 哪些模块后续还需要补结构字段

### 所能够调用的skill或者其他辅助项目的工具

- 文本检索
- 事件回调搜索
- 日志输出搜索
- 模块文件阅读

### 不做什么事情

- 不在这一步改动 UI 外观
- 不引入新的状态层
- 不设计 HID 协议

### 查验标准

- 能清楚指出每个控件的事件来源
- 能清楚指出每个控件是否仍在走 demo 日志通道
- 能清楚指出后续哪些模块优先接入统一状态层

---

## Step 3：冻结一版“默认映射清单”

### 角色指定

你现在是“输入语义设计者”，这一步的职责是把 UI 模块和业务/手柄语义对齐。

### 项目背景信息

如果不先冻结默认映射，后面即使把状态层写出来，也会出现“这个按钮到底算 A 还是 X”“这个滑块到底是视角还是其他模拟量”的混乱。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.h`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.c`
- `E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`
- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\ESP32P4_虚拟手柄整合路线.md`

### 本步骤的任务

1. 确认左摇杆固定映射为移动输入
2. 确认右侧2D滑块固定映射为视角输入
3. 确认 DPad、A/B/X/Y、L1/L2、R1/R2 的默认语义
4. 预留 `START` 和 `SELECT` 的逻辑位置
5. 明确“默认映射”和“可编辑映射”是两层概念：
   - 默认映射是初始值
   - 编辑映射是运行时可修改配置

### 所能够调用的skill或者其他辅助项目的工具

- 文档整理
- 模块清单核对
- 预设枚举核对

### 不做什么事情

- 不直接开始写 mapper 代码
- 不在这一步做 slot 兼容改造
- 不急着支持全部业务动作

### 查验标准

- 有一张稳定的“默认映射清单”
- 团队内对左摇杆/右滑块的语义不再反复变动
- 后续状态层字段命名有明确依据

---

## Step 4：建立统一输入状态层

### 角色指定

你现在是“状态层实现者”，这一步的职责是建立整个系统后续共享的唯一输入状态源。

### 项目背景信息

当前所有控件各自输出各自的内容，没有统一收口。统一状态层是后续映射层、编辑器绑定层、输出层的共同基础。

### 相关的文件绝对路径

- 新增：`E:\Vscode_codes\lvgl\src\gamepad_input_state.h`
- 新增：`E:\Vscode_codes\lvgl\src\gamepad_input_state.c`
- 修改：`E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`

### 本步骤的任务

1. 新建 `gamepad_input_state_t`
2. 在结构里至少包含以下字段：
   - `move_x`
   - `move_y`
   - `look_x`
   - `look_y`
   - `btn_a`
   - `btn_b`
   - `btn_x`
   - `btn_y`
   - `btn_l1`
   - `btn_l2`
   - `btn_r1`
   - `btn_r2`
   - `btn_start`
   - `btn_select`
   - `dpad_up`
   - `dpad_down`
   - `dpad_left`
   - `dpad_right`
   - `dirty`
3. 提供统一 API：
   - `gamepad_input_state_reset()`
   - `gamepad_input_get_state()`
   - `gamepad_input_set_move(...)`
   - `gamepad_input_set_look(...)`
   - `gamepad_input_set_button(...)`
   - `gamepad_input_set_dpad(...)`
4. 在 `CMakeLists.txt` 里把这两个新文件编译进去

### 所能够调用的skill或者其他辅助项目的工具

- 头文件设计
- 状态结构设计
- CMake接入

### 不做什么事情

- 不在这一步修改编辑器 UI
- 不在这一步引入 slot 绑定保存
- 不在这一步引入 HID report

### 查验标准

- 新增状态层可以单独编译通过
- 命名统一，字段语义清晰
- 后续任何控件都可以只通过这个层写入状态

---

## Step 5：把左摇杆正式接入统一状态层

### 角色指定

你现在是“左摇杆接入工程师”，这一步只负责把移动输入打通。

### 项目背景信息

左摇杆是你最终交互里最核心的输入之一，也是最适合最先打通的模块。先让移动轴稳定，再推进其他控件，风险最小。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\joystick_stick_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.h`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.c`

### 本步骤的任务

1. 找到左摇杆当前输出逻辑
2. 把 `printf("LEFT_STICK ...")` 从主逻辑中降级为可选调试输出
3. 在摇杆位置变化时调用 `gamepad_input_set_move(x, y)`
4. 在回弹到中心时，把 `move_x` / `move_y` 恢复到中立值
5. 统一坐标方向，明确：
   - 左右正负含义
   - 上下正负含义

### 所能够调用的skill或者其他辅助项目的工具

- 事件回调修改
- 输入归一化处理
- 调试日志辅助

### 不做什么事情

- 不改右侧滑块
- 不改 DPad
- 不改布局模型

### 查验标准

- 拖动左摇杆时，统一状态层中的移动向量实时变化
- 松手后，移动向量回到中立值
- 日志不再是唯一行为依据

---

## Step 6：把右侧2D滑块正式接入统一状态层

### 角色指定

你现在是“视角输入接入工程师”，这一步只负责把右侧视角控制打通。

### 项目背景信息

你已经明确右边滑块的职责是“视角转动”。所以这一步的本质是把 demo 里的 `VIEW_SLIDER` 输出，升级成稳定的 `look_x/look_y`。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\codex_gpt5_slider_2d_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.h`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.c`

### 本步骤的任务

1. 找到 `VIEW_SLIDER` 当前输出逻辑
2. 把当前 `printf` 输出改为 `gamepad_input_set_look(x, y)`
3. 明确视角输入的坐标方向
4. 明确中心点回归后的状态复位逻辑
5. 如果保留惯性动画，确认动画结束时状态也是正确的

### 所能够调用的skill或者其他辅助项目的工具

- 滑块事件回调修改
- 归一化处理
- 动画收尾验证

### 不做什么事情

- 不在这一步处理自定义绑定
- 不在这一步做输出层
- 不在这一步改 slot 保存格式

### 查验标准

- 右侧2D滑块拖动时，`look_x/look_y` 实时更新
- 惯性动画期间状态仍然正确
- 松手后如需回中，状态也同步回中

---

## Step 7：把按钮、肩键、DPad接入统一状态层

### 角色指定

你现在是“离散输入接入工程师”，这一步负责把所有按键类输入收口。

### 项目背景信息

除了两个模拟输入外，其余模块基本都属于按钮型或方向型输入。这一步完成后，虚拟手柄的大部分核心输入链路就完整了。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\gamepad_button_module.c`
- `E:\Vscode_codes\lvgl\src\shoulder_keys_demo.c`
- `E:\Vscode_codes\lvgl\src\Dpad.c`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.h`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.c`

### 本步骤的任务

1. 单按钮模块接入：
   - `A`
   - `B`
   - `X`
   - `Y`
2. 肩键模块接入：
   - `L1`
   - `L2`
   - `R1`
   - `R2`
3. DPad 模块接入：
   - `UP`
   - `DOWN`
   - `LEFT`
   - `RIGHT`
4. 明确按钮按下与抬起的状态更新方式
5. 确认 DPad 的 `OK` 中心点当前是否需要映射；若暂不需要，则先保持展示态

### 所能够调用的skill或者其他辅助项目的工具

- 事件回调接线
- 按键状态设计
- DPad行为核对

### 不做什么事情

- 不在这一步设计复杂组合键
- 不在这一步把 DPad 中心点强行映射到某个动作
- 不在这一步引入绑定编辑器

### 查验标准

- A/B/X/Y、L1/L2、R1/R2、DPad 四向都能写入统一状态
- 按下和松手状态能正确变化
- 所有核心控件不再依赖 `printf` 才算“工作”

---

## Step 8：增加一个屏幕调试观察层

### 角色指定

你现在是“联调可视化工程师”，这一步的目标是让状态变化能被肉眼看到。

### 项目背景信息

如果没有一个统一状态可视化层，后面你在调映射、调编辑器、调输出层时，会很难判断到底是哪里出了问题。

### 相关的文件绝对路径

- 优先修改：`E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`
- 可选新增：`E:\Vscode_codes\lvgl\src\gamepad_debug_overlay.h`
- 可选新增：`E:\Vscode_codes\lvgl\src\gamepad_debug_overlay.c`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.h`

### 本步骤的任务

1. 在页面上增加一个调试状态显示区域
2. 让它实时显示：
   - `move_x`, `move_y`
   - `look_x`, `look_y`
   - `A/B/X/Y`
   - `L1/L2/R1/R2`
   - `DPad`
3. 让调试层不会影响编辑器主要交互
4. 让后续 mapper 和 output 层也能借这个区域确认数据流是否正确

### 所能够调用的skill或者其他辅助项目的工具

- LVGL标签刷新
- 状态可视化
- UI联调

### 不做什么事情

- 不在这一步加入太多复杂动画
- 不在这一步重新设计整套界面
- 不用调试面板替代正式输出层

### 查验标准

- 手指操作控件时，屏幕上能同步看到状态变化
- 调试层能帮助区分“控件没触发”和“状态没更新”这两类问题

---

## Step 9：扩展布局模型，支持动作绑定字段

### 角色指定

你现在是“布局配置模型设计者”，这一步负责把“外观布局”升级成“布局 + 行为绑定”。

### 项目背景信息

当前 `gamepad_module_instance_t` 里主要是：

- `id`
- `kind`
- `preset_id`
- `x`
- `y`
- `w`
- `h`
- `visible`

这还不足以支撑“用户自定义编辑”。因为用户真正需要改的，除了位置大小，还有“这个模块输出什么动作”。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.h`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.c`

### 本步骤的任务

1. 为 `gamepad_module_instance_t` 增加绑定字段，建议至少包含：
   - `action_id`
   - `locked`
   - `deadzone`
   - `sensitivity`
2. 新增动作枚举，建议至少包含：
   - `GAMEPAD_ACTION_NONE`
   - `GAMEPAD_ACTION_MOVE`
   - `GAMEPAD_ACTION_LOOK`
   - `GAMEPAD_ACTION_DPAD`
   - `GAMEPAD_ACTION_BTN_A`
   - `GAMEPAD_ACTION_BTN_B`
   - `GAMEPAD_ACTION_BTN_X`
   - `GAMEPAD_ACTION_BTN_Y`
   - `GAMEPAD_ACTION_BTN_L1`
   - `GAMEPAD_ACTION_BTN_L2`
   - `GAMEPAD_ACTION_BTN_R1`
   - `GAMEPAD_ACTION_BTN_R2`
   - `GAMEPAD_ACTION_BTN_START`
   - `GAMEPAD_ACTION_BTN_SELECT`
3. 给默认布局加载逻辑补上默认绑定
4. 给 JSON 保存/加载逻辑补上新字段
5. 处理旧 slot 配置的兼容加载策略

### 所能够调用的skill或者其他辅助项目的工具

- 数据模型设计
- JSON序列化字段扩展
- 兼容性设计

### 不做什么事情

- 不在这一步直接写 UI 绑定编辑器
- 不在这一步直接改控件事件逻辑
- 不在这一步输出 HID 报文

### 查验标准

- 一个模块除了位置大小之外，还能描述“它控制什么”
- 新旧 slot 读取不会立刻全部失效
- 新保存的配置能恢复动作绑定信息

---

## Step 10：建立运行时动作映射层

### 角色指定

你现在是“动作路由设计者”，这一步负责把控件事件与最终状态更新解耦。

### 项目背景信息

如果控件内部直接写死“这个就是 A、那个就是移动”，那么编辑器后面改绑定就没有意义。必须新增一层 mapper，把“模块是谁”和“动作是什么”分开。

### 相关的文件绝对路径

- 新增：`E:\Vscode_codes\lvgl\src\gamepad_action_mapper.h`
- 新增：`E:\Vscode_codes\lvgl\src\gamepad_action_mapper.c`
- 修改：`E:\Vscode_codes\lvgl\src\joystick_stick_demo.c`
- 修改：`E:\Vscode_codes\lvgl\src\codex_gpt5_slider_2d_demo.c`
- 修改：`E:\Vscode_codes\lvgl\src\Dpad.c`
- 修改：`E:\Vscode_codes\lvgl\src\shoulder_keys_demo.c`
- 修改：`E:\Vscode_codes\lvgl\src\gamepad_button_module.c`
- 修改：`E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`

### 本步骤的任务

1. 定义 mapper 接口，让事件不再直接写状态层
2. 让 mapper 接收：
   - 模块实例信息
   - 当前绑定动作
   - 当前输入值
3. 由 mapper 决定写入哪一类状态：
   - 模拟轴
   - DPad
   - 按钮
4. 支持“同一个按钮 UI，通过改绑定，变成不同动作”

### 所能够调用的skill或者其他辅助项目的工具

- 中间层设计
- 控件解耦
- 运行时路由逻辑设计

### 不做什么事情

- 不把 mapper 直接做成 UI 层的一部分
- 不把 mapper 和 HID 输出层耦合
- 不在这一步扩展太多业务特例

### 查验标准

- 控件本身不再等于最终动作
- 修改绑定后，实际运行行为也会跟着改变
- 控件层、状态层、映射层职责分离

---

## Step 11：让编辑模式支持修改动作绑定

### 角色指定

你现在是“手柄配置编辑器实现者”，这一步的职责是把编辑模式从“改布局”升级成“改布局 + 改映射”。

### 项目背景信息

你已经明确项目后面要有“自定义编辑”的能力。如果编辑模式不能改绑定，那这个编辑器就只是排版器，不是完整的虚拟手柄配置器。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.h`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.c`
- `E:\Vscode_codes\lvgl\src\gamepad_action_mapper.h`
- `E:\Vscode_codes\lvgl\src\gamepad_action_mapper.c`

### 本步骤的任务

1. 为选中模块增加“当前绑定显示”
2. 增加一个简单的绑定选择界面
3. 允许用户修改当前模块的 `action_id`
4. 修改后立即写回当前布局配置
5. 保存 slot 时一并保存绑定信息
6. 加载 slot 时一并恢复绑定信息

### 所能够调用的skill或者其他辅助项目的工具

- LVGL表单/面板设计
- 当前选中模块信息联动
- 配置写回逻辑

### 不做什么事情

- 不在这一步重新设计整套视觉风格
- 不一次性把所有高级参数编辑都做完
- 不做复杂多页配置器

### 查验标准

- 至少按钮类模块可以改绑定
- 改完保存后，再加载仍能恢复
- 改绑定会真实影响运行结果

---

## Step 12：建立输出层接口

### 角色指定

你现在是“输出层抽象设计者”，这一步负责把统一输入状态导出给外部使用者。

### 项目背景信息

统一状态层本身还只是内部数据。后面无论你要接业务控制、脚本、协议、还是 HID，都需要一个清晰的输出层。

### 相关的文件绝对路径

- 新增：`E:\Vscode_codes\lvgl\src\gamepad_output.h`
- 新增：`E:\Vscode_codes\lvgl\src\gamepad_output.c`
- 可选新增：`E:\Vscode_codes\lvgl\src\gamepad_hid_report.h`
- 可选新增：`E:\Vscode_codes\lvgl\src\gamepad_hid_report.c`
- 修改：`E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`

### 本步骤的任务

1. 定义统一输出结构
2. 定义状态层到输出层的转换函数
3. 先做一个本地业务友好的输出版本
4. 预留 HID report 适配层接口
5. 保证输出层不反向耦合 UI 控件

### 所能够调用的skill或者其他辅助项目的工具

- 接口抽象
- 状态转换逻辑
- 后续协议适配预留

### 不做什么事情

- 不在这一步直接做完整 USB/BLE HID 协议
- 不在这一步把输出层写死为某一个业务
- 不让页面代码直接承担输出协议职责

### 查验标准

- 统一状态可以通过一个标准接口导出
- 业务层不需要知道具体 UI 控件细节
- 以后接 HID 时不需要回头重写 UI

---

## Step 13：做一次完整联调与阶段验收

### 角色指定

你现在是“系统联调负责人”，这一步负责验证整条链路是否真的闭环。

### 项目背景信息

只有当“控件 -> 状态 -> 映射 -> 编辑配置 -> 输出”这条链路都能跑通，这个项目才算真正进入可用状态。

### 相关的文件绝对路径

- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\main.c`
- `E:\Vscode_codes\lvgl\WT9932P4-TINY\lvgl_demo_v9\main\CMakeLists.txt`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.h`
- `E:\Vscode_codes\lvgl\src\gamepad_input_state.c`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.h`
- `E:\Vscode_codes\lvgl\src\gamepad_layout_model.c`
- `E:\Vscode_codes\lvgl\src\gamepad_action_mapper.h`
- `E:\Vscode_codes\lvgl\src\gamepad_action_mapper.c`
- `E:\Vscode_codes\lvgl\src\xbox_touch_gamepad_layout_demo.c`
- `E:\Vscode_codes\lvgl\src\gamepad_output.h`
- `E:\Vscode_codes\lvgl\src\gamepad_output.c`

### 本步骤的任务

1. 启动 ESP32-P4 工程
2. 检查页面是否能正常显示
3. 检查编辑模式与播放模式切换是否正常
4. 检查左摇杆和右滑块的实时状态输出
5. 检查 A/B/X/Y、L1/L2、R1/R2、DPad 状态变化
6. 检查修改绑定后的运行结果是否正确
7. 检查保存/加载 slot 后，布局和绑定是否都能恢复
8. 检查输出层是否能收到统一状态结果

### 所能够调用的skill或者其他辅助项目的工具

- 联调
- 串口日志观察
- 页面交互验证
- slot保存加载验证

### 不做什么事情

- 不在联调阶段临时大改架构
- 不把发现的问题直接混成新的需求扩展
- 不跳过保存/加载验证

### 查验标准

- 整体工程可正常运行
- 页面交互正常
- 自定义绑定正常
- slot恢复正常
- 输出层有稳定结果

---

## 推荐实施顺序

如果你现在马上开始动手，建议顺序严格按下面来：

1. `Step 1` 锁定宿主工程
2. `Step 2` 盘点模块现状
3. `Step 3` 冻结默认映射
4. `Step 4` 建统一状态层
5. `Step 5` 接左摇杆
6. `Step 6` 接右侧2D滑块
7. `Step 7` 接按钮/肩键/DPad
8. `Step 8` 加调试观察层
9. `Step 9` 扩布局模型
10. `Step 10` 加动作映射层
11. `Step 11` 编辑器支持改绑定
12. `Step 12` 加输出层
13. `Step 13` 做完整联调

---

## 最终阶段验收标准

当下面这些条件都满足时，这条路线就算真正跑通：

- `ESP32-P4` 工程能稳定编译运行
- `LVGL` 页面能稳定显示
- 左摇杆稳定输出移动向量
- 右侧2D滑块稳定输出视角向量
- 按钮、肩键、DPad 全部进入统一状态层
- 编辑模式不仅能改布局，还能改绑定
- slot 保存/加载能恢复布局与绑定
- 输出层可稳定提供统一手柄状态

---

## 一句话总结

你现在真正要做的，不是“重新做一个手柄UI”，而是：

**把已经接入 `ESP32-P4` 工程的 LVGL 可编辑手柄界面，升级成一套有统一状态层、支持动作绑定、支持配置保存、并且可向业务层或 HID 层输出的虚拟手柄系统。**
