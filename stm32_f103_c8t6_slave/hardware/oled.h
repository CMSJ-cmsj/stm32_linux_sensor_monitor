#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x.h"
#include "i2c.h"

/**
 * @brief OLED I2C从机写地址
 * @note 市面上0.96寸SSD1306 OLED模块两种地址：
 *       0x78 最常用；屏幕不亮、无应答时尝试改成 0x7A
 */
#define OLED_SLAVE_ADDR_WR     0x78
// SSD1306控制字节定义，I2C数据包第二个字节
#define OLED_CTRL_CMD          0x00    // 后面跟随的字节是【寄存器命令】
#define OLED_CTRL_DATA         0x40    // 后面跟随的字节是【显存像素数据】

/**
 * @brief OLED初始化
 * @param hi2c 使用的软件I2C实例指针
 * @attention 调用前必须已经初始化好对应的I2C外设
 */
void OLED_Init(SoftI2C_TypeDef *hi2c);

/**
 * @brief OLED全屏清屏
 * @param hi2c 使用的软件I2C实例指针
 */
void OLED_Clear(SoftI2C_TypeDef *hi2c);

/**
 * @brief 设置SSD1306页模式下光标位置
 * @param hi2c 使用的软件I2C实例指针
 * @param Page：页地址，范围0~7
 * @param Column：列地址，范围0~127
 */
void OLED_SetCursor(SoftI2C_TypeDef *hi2c,uint8_t Page, uint8_t Column);

/**
 * @brief OLED显示单个ASCII字符
 * @param hi2c 使用的软件I2C实例指针
 * @param Line：行号，范围1~4
 * @param Column：列号，范围1~16
 * @param Char：待显示ASCII字符
 */
void OLED_ShowChar(SoftI2C_TypeDef *hi2c,uint8_t Line,uint8_t Column,char Char);

/**
 * @brief OLED显示字符串
 * @param hi2c 使用的软件I2C实例指针
 * @param Line：行号1~4
 * @param Column：起始列1~16
 * @param String 字符串指针
 */
void OLED_ShowString(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, char *String);

/**
 * @brief OLED显示无符号十进制数字
 * @param hi2c 使用的软件I2C实例指针
 * @param Line 行
 * @param Column 起始列
 * @param Num 待显示数字
 * @param Len 显示总位数
 */
void OLED_ShowNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len);

/**
 * @brief OLED显示带符号十进制数字
 * @param hi2c 使用的软件I2C实例指针
 * @param Line 行
 * @param Column 起始列
 * @param Num 带符号数字
 * @param Len 数字部分显示位数
 */
void OLED_ShowSignedNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, int32_t Num, uint8_t Len);

/**
 * @brief OLED显示十六进制数
 * @param hi2c 使用的软件I2C实例指针
 * @param Line 行
 * @param Column 起始列
 * @param Num 待显示数字
 * @param Len 十六进制字符位数
 */
void OLED_ShowHexNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len);

/**
 * @brief OLED显示二进制数字
 * @param hi2c 使用的软件I2C实例指针
 * @param Line 行
 * @param Column 起始列
 * @param Num 待显示数字
 * @param Len 二进制位宽
 */
void OLED_ShowBinNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len);

#endif
