#include "oled.h"
#include "oled_font.h"
#include "delay.h"

/**
 * @brief  向OLED SSD1306写入一条寄存器命令
 * @param  hi2c I2C实例指针
 * @param  cmd ：SSD1306寄存器命令码
 * @retval uint8_t：0=发送成功收到从机应答；1=设备无应答（接线/地址/硬件故障）
 * @note   I2C写操作完整时序：起始 → 从机写地址 → 控制字节(0x00=命令) → 命令字节 → 停止
 */
static uint8_t OLED_WriteCmd(SoftI2C_TypeDef *hi2c,uint8_t cmd)
{
    uint8_t ack_ret;
    Soft_I2C_Start(hi2c);                          // 第一步：I2C发起起始信号
    Soft_I2C_SendByte(hi2c,OLED_SLAVE_ADDR_WR);     // 发送OLED从机写地址
    ack_ret = Soft_I2C_WaitAck(hi2c);              // 等待OLED应答，判断设备是否存在
    if(ack_ret != 0)
    {
        Soft_I2C_Stop(hi2c);
        return 1; // OLED没有应答，直接结束通信，返回错误
    }
    Soft_I2C_SendByte(hi2c,OLED_CTRL_CMD);          // 控制字节0x00：告诉SSD1306下一个字节是命令
    ack_ret = Soft_I2C_WaitAck(hi2c);
    if(ack_ret != 0)
    {
        Soft_I2C_Stop(hi2c);
        return 1;
    }
    Soft_I2C_SendByte(hi2c,cmd);                    // 真正的SSD1306寄存器命令
    ack_ret = Soft_I2C_WaitAck(hi2c);
    Soft_I2C_Stop(hi2c);                           // I2C停止信号，结束本次传输
    return ack_ret;
}

/**
 * @brief  设置SSD1306页模式下光标位置
 * @param hi2c I2C实例指针
 * @param  Page：页地址，范围0~7；SSD1306 128*64屏幕，一共8页，每页8行像素
 * @param  Column：列地址，范围0~127，横向128个像素点
 * @note 【页地址模式】：我们工程使用页模式，适合字符串显示；
 *        一页8个像素高度；我们字库是8*16，一个字符占连续2页(Page 和 Page+1)
 */
void OLED_SetCursor(SoftI2C_TypeDef *hi2c,uint8_t Page, uint8_t Column)
{
    // 硬件参数合法性简单防护，防止越界
    if(Page > 7)    Page = 7;
    if(Column >127) Column =127;
    OLED_WriteCmd(hi2c,0xB0 + Page);                 // 设置目标页
    OLED_WriteCmd(hi2c,Column & 0x0F);               // 取出列地址低4位
    OLED_WriteCmd(hi2c,0x10 | ((Column >> 4) & 0x0F)); // 取出列地址高4位，带上0x10标记位
}

/**
 * @brief  向OLED显存写入像素数据（要显示的点阵）
 * @param hi2c I2C实例指针
 * @param  data ：8bit显存像素数据
 * @retval uint8_t：0成功；1设备无应答
 * @note   控制字节0x40：告诉SSD1306，后面字节是【显存像素数据】，不是寄存器命令
 */
static uint8_t OLED_WriteData(SoftI2C_TypeDef *hi2c,uint8_t data)
{
    uint8_t ack_ret;
    Soft_I2C_Start(hi2c);
    Soft_I2C_SendByte(hi2c,OLED_SLAVE_ADDR_WR);
    ack_ret = Soft_I2C_WaitAck(hi2c);
    if(ack_ret != 0)
    {
        Soft_I2C_Stop(hi2c);
        return 1;
    }
    Soft_I2C_SendByte(hi2c,OLED_CTRL_DATA);         // 0x40代表显存数据
    ack_ret = Soft_I2C_WaitAck(hi2c);
    if(ack_ret != 0)
    {
        Soft_I2C_Stop(hi2c);
        return 1;
    }
    Soft_I2C_SendByte(hi2c,data);
    ack_ret = Soft_I2C_WaitAck(hi2c);
    Soft_I2C_Stop(hi2c);
    return ack_ret;
}

/**
 * @brief OLED全屏清屏，把全部显存写0x00，所有像素熄灭
 * @param hi2c I2C实例指针
 * @note SSD1306页模式：循环遍历全部8页；每页从第0列到127列，全部写入0x00
 */
void OLED_Clear(SoftI2C_TypeDef *hi2c)
{
    uint8_t page_idx;
    uint8_t col_idx;
    for(page_idx = 0; page_idx < 8; page_idx++)
    {
        OLED_SetCursor(hi2c,page_idx, 0);          // 光标移动到这一页最左侧第0列
        for(col_idx = 0; col_idx < 128; col_idx++)
        {
            OLED_WriteData(hi2c,0x00);              // 写入0，关闭这一列8个像素
        }
    }
}

/**
 * @brief OLED SSD1306初始化
 * @param hi2c I2C实例指针
 * @note 江协原版寄存器配置全部保留
 */
void OLED_Init(SoftI2C_TypeDef *hi2c)
{
	// 模块上电稳定等待，部分OLED上电需要几十ms稳定时间
	delay_ms(100);

	// 【关闭显示】
	// 上电先关闭屏幕，配置完所有寄存器之后再打开，避免配置过程屏幕乱闪
	OLED_WriteCmd(hi2c,0xAE);

	// 设置显示时钟分频因子 + 振荡器频率
	OLED_WriteCmd(hi2c,0xD5);
	// 分频=1，振荡器频率默认，绝大多数0.96寸屏幕通用参数
	OLED_WriteCmd(hi2c,0x80);

	// 设置MUX多路复用率，也就是屏幕纵向驱动行数
	OLED_WriteCmd(hi2c,0xA8);
	// 0x3F=63，128*64屏幕纵向一共64行像素，所以设置64路复用
	OLED_WriteCmd(hi2c,0x3F);

	// 设置显示垂直偏移：画面整体上下偏移多少行
	OLED_WriteCmd(hi2c,0xD3);
	// 0：没有偏移，画面从最顶部开始显示
	OLED_WriteCmd(hi2c,0x00);

	// 设置显示RAM起始行，从显存第0行开始输出到屏幕
	OLED_WriteCmd(hi2c,0x40);

	// 【电荷泵设置 I2C‑OLED最关键命令，不能省略！】
	// I2C版本OLED模块没有外部VCC给显存驱动供电，必须开启芯片内部电荷泵升压
	OLED_WriteCmd(hi2c,0x8D);
	// 0x14：开启电荷泵；如果写0x10关闭电荷泵，屏幕直接黑屏
	OLED_WriteCmd(hi2c,0x14);

	// 设置显存地址模式
	OLED_WriteCmd(hi2c,0x20);
	// 0x02 = 页地址模式！
	// 页模式：写完一页之后，列自动增加；到页末尾不会自动跳到下一页，适合字符显示。
	OLED_WriteCmd(hi2c,0x02);

	// 段重映射：0xA1，显存第0列对应屏幕硬件最左侧；0xA0左右画面翻转
	OLED_WriteCmd(hi2c,0xA1);

	// COM扫描方向；0xC8从上往下扫描；0xC0画面上下颠倒
	OLED_WriteCmd(hi2c,0xC8);

	// COM硬件引脚配置，根据屏幕硬件布线选择参数
	OLED_WriteCmd(hi2c,0xDA);
	// 128*64屏幕固定0x12；128*32屏幕是0x02
	OLED_WriteCmd(hi2c,0x12);

	// 设置对比度，亮度
	OLED_WriteCmd(hi2c,0x81);
	// 0~0xFF，数值越大屏幕越亮；0xCF中等偏亮，日常使用
	OLED_WriteCmd(hi2c,0xCF);

	// 设置预充电周期，像素点亮前充电时间
	OLED_WriteCmd(hi2c,0xD9);
	OLED_WriteCmd(hi2c,0xF1);

	// VCOMH取消选择电平，COM引脚硬件电平配置
	OLED_WriteCmd(hi2c,0xDB);
	OLED_WriteCmd(hi2c,0x30);

	// 0xA4：根据显存RAM真实内容显示；0xA5强制全屏全部点亮，忽略显存
	OLED_WriteCmd(hi2c,0xA4);

	// 0xA6正常显示：显存bit=1像素点亮；0xA7反色显示，bit=0点亮
	OLED_WriteCmd(hi2c,0xA6);

	// 【打开显示】全部硬件寄存器配置完毕，正式开启屏幕输出
	OLED_WriteCmd(hi2c,0xAF);

	// 初始化完成，清空显存，屏幕全部熄灭，等待我们输出内容
	OLED_Clear(hi2c);
}

/**
 * @brief OLED显示单个ASCII字符，字库 8像素宽 ×16像素高
 * @param hi2c I2C实例指针
 * @param Line：行号，范围1~4。屏幕64像素高度，一个字符占16像素，一共4行字符。
 * @param Column：列号，范围1~16；横向128像素，一个字符占8像素，一行最多16个字符。
 * @param Char：要显示的ASCII可见字符，空格' ' ~ '~'；字库只覆盖这一段。
 */
void OLED_ShowChar(SoftI2C_TypeDef *hi2c,uint8_t Line,uint8_t Column,char Char)
{
	uint8_t page_up;         // 字符上半部分所在页
    uint8_t page_down;       // 字符下半部分所在页
    uint8_t font_index;      // 字库数组下标，字库以空格' '作为数组第0号元素
    uint8_t i;
    // 参数简单防护，防止越界
    if(Line < 1)    Line = 1;
    if(Line > 4)    Line = 4;
    if(Column < 1)  Column = 1;
    if(Column >16)  Column =16;
    page_up   = (Line - 1) * 2;       // 计算上半部分页号
    page_down = page_up + 1;          // 下半部分紧跟着下一页
    font_index = Char - ' ';          // 字库偏移，空格是0号

	// ========= 第一步：输出字符上半8行像素 =========
    OLED_SetCursor(hi2c,page_up, (Column - 1) * 8); // 光标移动到字符左上角位置
    for(i = 0 ; i < 8 ; i++)
    {
        OLED_WriteData(hi2c,OLED_F8x16[font_index][i]);
    }
    // ========= 第二步：输出字符下半8行像素 =========
    OLED_SetCursor(hi2c,page_down, (Column - 1) * 8);
    for(i = 0 ; i < 8 ; i++)
    {
        OLED_WriteData(hi2c,OLED_F8x16[font_index][i + 8]);
    }
}

/**
 * @brief OLED显示ASCII字符串
 * @param hi2c I2C实例指针
 * @param Line：行号1~4
 * @param Column：起始列号1~16
 * @param String：字符串指针，以'\0'结尾
 * @note 一行最多容纳16个字符；超过16会直接截断，不会自动换行
 */
void OLED_ShowString(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, char *String)
{
    // 循环，读到字符串结束符'\0'就退出
    while(*String != '\0')
    {
        OLED_ShowChar(hi2c,Line, Column, *String); // 打印当前这一个字符
        Column++;                             // 列号向后移动一格，准备打印下一个
        if(Column > 16)                       // 一行最多16个字符，超出直接截断
        {
            break;
        }
        String++;                             // 指针移动到下一个字符
    }
}

/**
 * @brief OLED显示无符号十进制数字
 * @param hi2c I2C实例指针
 * @param Line：行1‑4
 * @param Column：起始列1‑16
 * @param Num：要显示的数字 uint32_t
 * @param Len：一共显示几位；不足位数前面补空格
 */
void OLED_ShowNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len)
{
    uint8_t t;
    uint8_t temp_digit;
    uint8_t has_start = 0;   // 标记：是否已经遇到非0数字，用来处理前导空格
    for(t = 0; t < Len; t++)
    {
        // 取出当前最高位数字
        uint32_t divisor = 1;
        uint8_t k;
        for(k = 0; k < (Len - t - 1); k++)
        {
            divisor *= 10;
        }
        temp_digit = Num / divisor % 10;
        if(has_start == 0)
        {
            if(temp_digit == 0 && t != (Len - 1))
            {
                // 还没有遇到有效数字，并且不是最后一位：打印空格，消掉前导0
                OLED_ShowChar(hi2c,Line, Column + t, ' ');
                continue;
            }
            else
            {
                has_start = 1; // 已经碰到真实数字，后续正常输出
            }
        }
        OLED_ShowChar(hi2c,Line, Column + t, temp_digit + '0'); // 数字转ASCII字符
    }
}

/**
 * @brief OLED显示带符号十进制数字 int32_t
 * @param hi2c I2C实例指针
 * @param Line：行
 * @param Column：起始列
 * @param Num：有符号整数
 * @param Len：数字部分显示位数，负号会额外占一列
 */
void OLED_ShowSignedNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, int32_t Num, uint8_t Len)
{
    uint32_t num_abs;
    if(Num < 0)
    {
        OLED_ShowChar(hi2c,Line, Column, '-');
		num_abs = (uint32_t)(-Num);
		OLED_ShowNum(hi2c,Line, Column + 1, num_abs, Len-1);
		//Len减1，因为负号已经占掉1个字符位置
    }
    else
    {
        //正数不打印+号，直接显示数字
        num_abs = (uint32_t)Num;
        OLED_ShowNum(hi2c,Line, Column, num_abs, Len);
    }
}

/**
 * @brief  OLED显示十六进制数(移位高效版，消前导0，前导补空格)
 * @param hi2c I2C实例指针
 * @param  Line 		行
 * @param  Column		起始列X
 * @param  Num			待显示无符号数字 uint32_t
 * @param  Len			显示多少个十六进制字符
 */
void OLED_ShowHexNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len)
{
	uint8_t t;
	uint8_t hex_val;
	uint8_t has_start = 0;  //标记：是否已经遇到非0有效数字
	for(t = 0; t < Len; t++)
	{
		// 核心：右移，把当前要取的4bit挪到最低位，&0x0F抠出4bit
		hex_val = (Num >> (4 * (Len - t - 1))) & 0x0F;
		if(has_start == 0) //还没碰到有效数字，处在前导区域
		{
			// 当前是0，并且不是最后一位：输出空格，消掉前导0
			if(hex_val == 0 && t != (Len - 1))
			{
				OLED_ShowChar(hi2c,Line, Column + t, ' ');
				continue; //跳过本轮后面打印字符代码，直接进入下一轮循环
			}
			else
			{
				has_start = 1; //碰到真实有效数字，开启正常输出模式
			}
		}
		//把0~15转为ASCII字符
		if(hex_val < 10)
		{
			OLED_ShowChar(hi2c,Line, Column + t, hex_val + '0');
		}
		else
		{
			OLED_ShowChar(hi2c,Line, Column + t, hex_val - 10 + 'A');
		}
	}
}

/**
 * @brief OLED显示二进制数字
 * @param hi2c I2C实例指针
 * @param Line：行
 * @param Column：起始列
 * @param Num：数字
 * @param Len：二进制位宽
 */
void OLED_ShowBinNum(SoftI2C_TypeDef *hi2c,uint8_t Line, uint8_t Column, uint32_t Num, uint8_t Len)
{
    uint8_t t;
    for(t = 0; t < Len; t++)
    {
        uint8_t bit = (Num >> (Len - t - 1)) & 0x01;
        OLED_ShowChar(hi2c,Line, Column + t, bit + '0');
    }
}
