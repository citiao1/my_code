# OLED驱动库（SSD1306）使用手册

## 概述

本驱动库专为 **SSD1306** 芯片的 OLED 显示屏设计，基于 STM32 HAL 库与 I2C 通信协议。库采用**显存（GRAM）机制**，所有绘制操作先在内存中的 `OLED_GRAM` 数组完成，最后通过 `OLED_ShowFrame()` 统一刷新到屏幕，能有效避免闪烁并提高绘图效率。

### 使用流程

1. **硬件初始化**：STM32 上电后，等待约 100ms 待 OLED 电源稳定，调用 `OLED_Init()`。
2. **开始绘制**：调用 `OLED_NewFrame()` 清空显存。
3. **绘制内容**：调用 `OLED_DrawXXX()` 绘制图形，或 `OLED_PrintXXX()` 绘制文字。
4. **显示内容**：调用 `OLED_ShowFrame()` 将显存内容发送至屏幕显示。

### 坐标定义

- **X 轴**：水平方向，范围 `0` ~ `127`（对应 `OLED_COLUMN`）。
- **Y 轴**：垂直方向，范围 `0` ~ `63`（对应 `OLED_ROW`）。

------

## 一、 底层通信与初始化函数

### 1. `void OLED_Send(uint8_t *data, uint8_t len)`

- **功能**：底层数据发送函数，基于 I2C 发送数据到 OLED。
- **注意**：若移植到非 STM32 平台或非 HAL 库环境，**必须重写此函数**。

### 2. `void OLED_Init()`

- **功能**：初始化 SSD1306 控制器，配置扫描方向、对比度、电荷泵等寄存器。
- **注意**：调用前确保 I2C 总线已初始化。建议 STM32 启动后延时 10~20ms 再调用。

### 3. `void OLED_DisPlay_On() / OLED_DisPlay_Off()`

- **功能**：开启或关闭 OLED 显示（仅控制屏幕亮灭，不丢失显存数据）。

### 4. `void OLED_SetColorMode(OLED_ColorMode mode)`

- **功能**：设置全局颜色模式。
- **参数**：
  - `OLED_COLOR_NORMAL`：黑底白字（正常模式）。
  - `OLED_COLOR_REVERSED`：白底黑字（反色模式）。

------

## 二、 显存管理函数（核心绘图逻辑）

### 1. `void OLED_NewFrame()`

- **功能**：清空内部显存 `OLED_GRAM`，准备绘制新的一帧。

### 2. `void OLED_ShowFrame()`

- **功能**：将当前显存数据通过 I2C 发送到 OLED 屏幕，完成显示更新。
- **注意**：此函数包含针对 SSD1306 的分页和列地址设置指令。若移植到其他驱动芯片（如 SH1106），需调整此函数。

### 3. 像素与位操作函数

| 函数名          | 功能描述                                  | 适用场景                         |
| :-------------- | :---------------------------------------- | :------------------------------- |
| `OLED_SetPixel` | 绘制单个像素点                            | 绘制离散点、简单图形调试         |
| `OLED_SetByte`  | 设置显存中完整的一个字节（8个垂直像素点） | 底层字体绘制、快速填充整列       |
| `OLED_SetBits`  | 设置从 (x, y) 开始向下的 8 个像素位       | 绘制 8bit 高度的字符或图标       |
| `OLED_SetBlock` | 绘制一块矩形区域（支持任意宽高）          | **绘制图像、大号字体的核心函数** |

------

## 三、 图形绘制函数

所有图形函数均绘制在显存中，需配合 `OLED_ShowFrame()` 显示。

| 函数名                     | 功能                                   | 关键参数                 |
| :------------------------- | :------------------------------------- | :----------------------- |
| `OLED_DrawLine`            | 画直线（支持任意斜率，Bresenham 算法） | `x1, y1, x2, y2`         |
| `OLED_DrawRectangle`       | 画空心矩形边框                         | `x, y, w, h`             |
| `OLED_DrawFilledRectangle` | 画实心填充矩形                         | `x, y, w, h`             |
| `OLED_DrawCircle`          | 画空心圆                               | `x, y, r`                |
| `OLED_DrawFilledCircle`    | 画实心圆                               | `x, y, r`                |
| `OLED_DrawEllipse`         | 画椭圆（空心）                         | `x, y, a(长轴), b(短轴)` |
| `OLED_DrawImage`           | 绘制预先取模的图片数据                 | `x, y, Image指针`        |

### 示例：绘制一个实心圆

c

```
OLED_NewFrame();
OLED_DrawFilledCircle(64, 32, 20, OLED_COLOR_NORMAL);
OLED_ShowFrame();
```



------

## 四、 文字绘制函数

文字绘制依赖于**字模数据**，字模需使用 **PCtoLCD** 或 **波特律动 LED 取模工具** 生成。

### 1. ASCII 字符绘制

c

```
void OLED_PrintASCIIChar(uint8_t x, uint8_t y, char ch, const ASCIIFont *font, OLED_ColorMode color);
void OLED_PrintASCIIString(uint8_t x, uint8_t y, char *str, const ASCIIFont *font, OLED_ColorMode color);
```



- **适用**：仅包含英文字符、数字、标点。
- **要求**：需定义 `ASCIIFont` 结构体字模。

### 2. 混合字符串绘制（支持中文）

c

```
void OLED_PrintString(uint8_t x, uint8_t y, char *str, const Font *font, OLED_ColorMode color);
```



- **功能**：自动识别 UTF-8 编码的中英文混合字符串。遇到中文查找 `font` 字库，遇到英文调用关联的 `font->ascii` 字库。
- **重要配置**：
  1. 编译器字符集必须设置为 **UTF-8**。
  2. 需定义包含中文索引表和字模数据的 `Font` 结构体。

### 示例：显示混合字符串

c

```
// 假设已准备好 Font 字体变量 "myFont"
OLED_NewFrame();
OLED_PrintString(0, 0, "温度: 25.6°C", &myFont, OLED_COLOR_NORMAL);
OLED_ShowFrame();
```



------

## 五、 移植注意事项

若要将此驱动移植到其他平台（非 STM32 HAL）或使用其他通信方式（如软件 I2C、SPI）：

1. **重写 `OLED_Send` 函数**：这是唯一的硬件接口依赖点。
2. **修改 `OLED_Init` 和 `OLED_ShowFrame`**：不同驱动 IC（如 SH1106、SSD1309）的初始化命令序列和寻址方式略有不同，需根据目标 IC 数据手册调整。
3. **头文件依赖**：确保 `oled.h` 中定义的数据类型（如 `uint8_t`）在当前平台可用（通常包含 `<stdint.h>`）。

------

## 六、 数据结构定义参考

为了配合绘图函数，您需要按以下格式准备图片或字模数据：

c

```
// 图片结构体
typedef struct {
    uint8_t w;      // 宽度
    uint8_t h;      // 高度
    const uint8_t *data; // 列行式排列的位图数据
} Image;

// ASCII 字体结构体
typedef struct {
    uint8_t w;      // 字符宽度
    uint8_t h;      // 字符高度
    const uint8_t *chars; // 字模数据起始地址（ASCII 从空格 0x20 开始）
} ASCIIFont;

// 混合字体结构体（用于 OLED_PrintString）
typedef struct {
    uint8_t w;              // 字符宽度
    uint8_t h;              // 字符高度
    uint8_t len;            // 包含的中文字符个数
    const ASCIIFont *ascii; // 关联的英文字体
    const uint8_t *chars;   // 字模索引表 + 数据表
} Font;
```