# STM32F103-C8T6 下位机串口通信模块调试复盘记录
> 工程：`stm32_f103_c8t6_slave`（下位机）
> 上位机：`stm32_f103_vet6_uphost`（Linux上位机，本阶段未开发）
> 文档存放路径：`log_record/DebugRecord.md`
> 现象截图存放：`log_record/`目录，本md使用相对路径引用。

## 1 模块概述
### 1.1 通信协议V3
- 帧头：`@`；帧尾：固定双字符`#!`；
- 报文格式：`@载荷#!`；`#!`不进入业务载荷；
- 接收模型：**中断仅完成字节接收、状态机流转；主循环做指令解析**；单帧全局缓冲区；
- 业务能力：串口指令控制LED、蜂鸣器、SG90舵机；设置温度报警阈值；读取MPU6050+ADC光照传感器。

### 1.2 硬件环境
- MCU：STM32F103C8T6；USART1，波特率9600；
- 外设：软件I2C‑OLED0.96寸；MPU6050；ADC光照采集；SG90舵机；LED、蜂鸣器。

## 2 初始故障现象（bug原始现象）
> 对应截图：![bug_garbled_print_01.jpg](./bug_garbled_print_01.jpg)

现象矛盾特征：
1. 上位机下发合法完整报文`@LED:ON#!`；
2. 状态机可以正确识别帧头帧尾；`strncmp`前缀匹配成功，外设动作可以正确执行（LED可以点亮）；
3. 但是使用`printf("%s")`打印收到的载荷，输出方框乱码；
4. 偶发舵机/阈值sscanf解析得到随机数值；
5. 硬件层面没有丢字节，硬件接收流程无异常；
6. 故障属于**概率性复现**，有时候完全正常，偶发乱码，静态阅读业务状态机跳转逻辑看不出语法错误。

> 最初错误猜想：怀疑状态机帧解析、怀疑sscanf库函数、怀疑缓冲区拷贝；先后尝试开发快照调试脚手架，在调试过程中**调试工具本身引入次生bug**。

## 3 第一轮调试：快照脚手架方案（引入次生故障）
> 目的：希望区分“中断收到数据是否正确”、“主循环处理前内存是否被踩踏”；
> 实现思路：ISR收到完整`#!`帧尾瞬间，把`Serial_RxPacket`memcpy拷贝到独立快照缓冲区；主循环打印快照与原始缓冲区做对比。

> ⚠️**这一套脚手架本身引入新的次生bug：不要直接在业务版本使用，仅作为历史调试记录**。

### 3.1 当时改动代码片段 usart.c（历史调试版本，业务版本已全部删除）
```c
// 调试新增全局快照缓冲区
char Serial_RxSnapshot[SERIAL_RX_BUF_LEN];
volatile uint8_t g_snapshot_valid = 0U;

//IRQ内部完整帧分支
if(RxData == '!')
{
    Serial_RxPacket[pRxPacket] = '\0';
    //快照拷贝：直接拷贝全部100字节缓冲区
    memcpy(Serial_RxSnapshot, Serial_RxPacket, SERIAL_RX_BUF_LEN);
    g_snapshot_valid = 1U;

    Serial_RxFlag = 1U;
    RxState = STATE_IDLE;
    pRxPacket = 0U;
}
### 3.2 main.c 主循环打印快照片段（历史调试代码）
```
if(g_snapshot_valid == 1U)
{
    Serial_Printf("[SNAPSHOT_IRQ] [%s]\r\n",Serial_RxSnapshot);
    g_snapshot_valid = 0U;
}
```
1. 仅仅手动发送**一帧指令**，串口疯狂循环重复打印快照；
2. 出现变量打印出超大随机数（`536872144`），远大于缓冲区 0~99 正常下标范围；
3. 后续定位：
   - 问题 1：快照标记采用「中断置 1，主循环清零」模式；在串口中断抢占主循环 printf 阻塞窗口，标记发生竞态，造成重复刷屏；
   - 问题 2：`memcpy`拷贝整个 100 字节缓冲区，缓冲区后半段残留脏内存；`%s`打印直到遇见全局随机`\0`，产生假乱码；
   - 问题 3：后续又尝试在 Cmd_Dispatch 内部写 for 循环大量 hex dump 打印，`Serial_Printf`内部局部栈数组`char String[100]`，原始使用无边界`vsprintf`，输出过长直接**栈溢出，破坏函数返回地址，程序跑飞**。

> 
> 教训记录：
> 
> 
> 1. 裸机 MCU，**不要在解析回调函数中做大批量格式化串口打印**，极易触发栈溢出；
> 2. 调试脚手架本身也会制造 bug；当调试工具输出现象混乱，优先全部移除脚手架，回归最小观测手段；
> 3. `printf("%s")`强依赖`\0`终止符，**不能作为判断缓冲区原始字节是否正确的唯一依据**。

## 4 第二轮调试：放弃串口快照，改用 OLED 硬件观测（历史调试版本代码）

> 
> 设计思路：
> 
> 
> - 不再依赖串口 printf 做第一手证据；
> - 将中断内部`static RxState、static pRxPacket`**迁移到全局 volatile**；
> - 主循环 OLED 屏幕直接显示中断状态机变量，硬件 I2C 屏幕输出，不会产生栈溢出；
> - 增加`g_last_frame_len`，ISR 完整帧瞬间冻结载荷长度；OLED 屏幕展示`St、Pkt、L、Flag`。

### 4.1 历史调试版本 usart.c 关键片段
// 从IRQ内部static迁移为全局volatile uint16_t
#define STATE_IDLE         0U
#define STATE_RECV_DATA    1U
#define STATE_WAIT_EXCL    2U
volatile uint16_t RxState = STATE_IDLE;
volatile uint16_t pRxPacket = 0U;

//调试变量
volatile uint8_t  dbg_RxState = 0U;
volatile uint8_t  dbg_RxData = 0U;
volatile uint16_t dbg_pRxPacket = 0U;
volatile uint8_t  dbg_Serial_RxFlag = 0U;
volatile uint8_t  dbg_EnterDefault = 0U;
volatile uint16_t g_last_frame_len = 0U;

4.2 中断完整帧分支（调试版）
if(RxData == '!')
{
    if(pRxPacket >= SERIAL_RX_BUF_LEN)
    {
        RxState = STATE_IDLE;
        pRxPacket = 0U;
        uart_frame_discard_warn = 1U;
        break;
    }
    Serial_RxPacket[pRxPacket] = '\0';
    g_last_frame_len = pRxPacket; //ISR冻结载荷长度，用于OLED显示
    Serial_RxFlag = 1U;
    RxState = STATE_IDLE;
    pRxPacket = 0U;
}

//每字节中断末尾，把状态拷贝给dbg_*，给OLED显示
dbg_RxState       = (uint8_t)RxState;
dbg_RxData        = RxData;
dbg_pRxPacket     = pRxPacket;
dbg_Serial_RxFlag = Serial_RxFlag;

4.3 main.c OLED 调试面板完整代码（**业务版本已经全部删除，仅历史归档**）
if(tim_period_flag == 1U)
{
    tim_period_flag = 0U;
    OLED_Clear(&oledi2c1);
    OLED_ShowString(&oledi2c1,1,1,"St=");
    OLED_ShowNum(&oledi2c1,1,4, RxState,1);
    OLED_ShowString(&oledi2c1,1,6,"D=0x");
    OLED_ShowHexNum(&oledi2c1,1,10, dbg_RxData,2);

    OLED_ShowString(&oledi2c1,2,1,"Pkt=");
    OLED_ShowNum(&oledi2c1,2,5, pRxPacket,4);
    OLED_ShowString(&oledi2c1,2,11,"L=");
    OLED_ShowNum(&oledi2c1,2,13, g_last_frame_len,4);

    OLED_ShowString(&oledi2c1,3,1,"Flag=");
    OLED_ShowNum(&oledi2c1,3,6, Serial_RxFlag,1);
    OLED_ShowString(&oledi2c1,3,9,"Def=");
    OLED_ShowNum(&oledi2c1,3,13, dbg_EnterDefault,1);

    OLED_ShowString(&oledi2c1,4,1,"FSM @ #!");
}

### 4.4 根因定位（本次项目最核心 bug）

> 
> 现象：调试版本中 OLED 观测，偶发`pRxPacket/g_last_frame_len`变成超大随机数，远大于缓冲区上限。

✅**根因：USART 中断服务函数内部`static uint16_t RxState、static uint16_t pRxPacket`**

1. C 语法层面：函数内部`static`变量，存储在全局 RAM，生命周期等于整个程序运行；**但是变量的作用域仅局限在 IRQ 函数内部**。
2. Keil‑ARMCC 编译器行为：即使 O0 零优化，编译器**不知道硬件中断会异步改写这个 static 局部变量**；编译器会把 static 局部变量缓存到 CPU 寄存器，不会每一次中断都去真实 RAM 读取。
3. 中断是硬件异步抢占；当发生中断抢占主循环时，寄存器缓存的值和真实 RAM 变量发生割裂，**产生未定义行为：pRxPacket 随机野值**。
4. 业务状态机跳转逻辑、帧头帧尾匹配算法**语法完全正确，不是业务算法 bug**；是**嵌入式 C 语言编译器 + 硬件中断异步访问的工程级隐性坑**。
5. 现象概率性复现：时序巧合的时候寄存器缓存值刚好等于期望值，程序完全正常；时序错位就出现 pRxPacket 野值，`Serial_RxPacket[pRxPacket] = '\0'`写入到错误下标，C 字符串终止符错位。
   - → `strncmp`只对比前缀 N 个字节，前缀字节是正确，匹配成功，外设动作正常；
   - → `printf("%s")`从起始地址一直读到内存随机`\0`，中间夹杂脏字节，打印方框乱码；
   - → sscanf 拿到错位终止符，解析出随机数值。

✅**修复手段（业务版本采用）**

> 
> 不改动**任何一行状态机跳转业务逻辑**；仅仅改变变量存储位置与 volatile 修饰：
> 将`RxState、pRxPacket`从 IRQ 内部 static 局部变量，**迁移到文件全局域，显式加上`volatile uint16_t`**。
> 
> 
> 1. `volatile`强制每一次读写都访问真实 SRAM，禁止编译器寄存器缓存；
> 2. 全局域，调试器 / OLED 可以直接观测；
> 3. ⚠️业务版本**不回退 IRQ 内部 static**，避免再次踩坑。

> 
> 业务版本保留的健壮性防御（必须保留）
//完整收到!帧尾分支，下标越界防御
if(pRxPacket >= SERIAL_RX_BUF_LEN)
{
    RxState = STATE_IDLE;
    pRxPacket = 0U;
    uart_frame_discard_warn = 1U;
    break;
}
Serial_RxPacket[pRxPacket] = '\0';

## 5 业务版本清理与取舍规则

> 
> 业务版本`stm32_f103_c8t6_slave`：只保留可交付业务实现，全部调试脚手架删除；调试历史全部归档本 md。

### 5.1 业务版本保留项

1. ✅三段状态机完整业务逻辑：`@`帧头、`#!`帧尾、载荷不写入帧尾字符；
2. ✅全局`volatile uint16_t RxState、pRxPacket`（规避本次核心 bug，业务代码不回退 static）；
3. ✅帧下标越界防御判断；
4. ✅业务告警`uart_frame_discard_warn`：`#`后面跟非`!`帧丢弃告警（协议业务能力，不是调试）；
5. ✅所有指令 handler、参数校验、sscanf 解析、应答`Resp:xxx OK#`；
6. ✅`Serial_Printf`替换为`vsnprintf(String,sizeof(String),format,ap)`；缓冲区边界防护；**输出行为和原版完全一致，仅增加截断防护，不会改变业务逻辑**；
7. ✅main 主循环恢复业务定时任务：传感器采集、报警逻辑、上电默认打开`App_Oled_Update`业务 OLED 界面；
8. ✅全部底层驱动 i2c/oled/mpu6050/adc/beep/led/tim/pwm 维持原始工程。

### 5.2 业务版本彻底删除项

1. ❌删除全部 dbg_* 调试全局变量；
2. ❌删除`g_last_frame_len`、快照`Serial_RxSnapshot`整套调试脚手架；
3. ❌删除 main.c 整块 OLED 状态机调试面板代码；
4. ❌删除所有`[DBG]`开头的调试串口打印、dump 打印；
5. ❌删除所有快照 memcpy 调试代码。

## 6 遗留风险与后续优化建议（毕设论文可以引用）

> 
> 当前业务版本已经可以稳定运行，下面属于后续可迭代优化点，本次交付不改动。

1. **单帧缓冲区风险**：当前是单帧全局缓冲区；上位机高速连续下发报文，会出现帧覆盖丢帧；后续可以升级串口接收环形 FIFO；本项目人手逐条下发指令，满足实训需求。
2. **半帧无超时复位**：如果串口物理断线，状态机会卡在`STATE_RECV_DATA / STATE_WAIT_EXCL`，后续报文无法接收；后续可以增加软件定时器超时，强制切回 IDLE；也可以使用看门狗。
3. **sscanf 浮点 % f MicroLib 坑**：Keil MicroLib 下`%f`存在兼容性风险；后续产品可以手写字符串解析，完全绕开库函数。
4. 虽然已经增加`vsnprintf`边界防护；业务代码中依然尽量避免在解析回调中做大批量格式化输出。
5. 工程编码规范：**STM32 裸机中断中，跨多次硬件中断调用、需要持久保存的状态，禁止使用函数内部 static 变量；优先全局 + volatile**。

## 7 回归测试用例（完整用例见同目录`TestCases.md`）

表格

| 测试用例 | 下发报文 | 业务版本预期现象 |
| --- | --- | --- |
| LED 打开 | `@LED:ON#!` | LED 点亮；应答`Resp:LED ON OK#` |
| LED 关闭 | `@LED:OFF#!` | LED 熄灭；应答`Resp:LED OFF OK#` |
| 舵机设置 90° | `@SG90:90#!` | 舵机转到 90°；应答参数 OK |
| 非法帧 #后非！ | `@ABC#X` | 打印业务 WARN 帧丢弃；不执行业务动作 |
| 空合法帧 | `@#!` | 返回`Resp:UNKNOWN CMD#`，无外设动作 |
| 超长载荷接近缓冲区上限 | 接近 99 字节长报文 | 触发溢出丢弃，状态机回到 IDLE |

## 8 踩坑总结（实训报告【调试分析】章节可以直接摘抄）

1. 现象具有迷惑性：**前缀 strncmp 匹配成功，但是 printf% s 打印乱码，不能直接判定接收硬件 / 状态机算法出错**；根源是 C 字符串`\0`终止符错位。
2. 不要盲目的一上来就写大量快照、dump 调试脚手架；**调试工具本身会引入次生 bug，干扰真实故障定位**；优先选用不依赖串口 printf 的硬件观测手段（OLED 屏幕）。
3. 嵌入式 C 语言一个非常隐蔽坑：**中断函数内部 static 持久状态变量，编译器寄存器缓存 + 硬件异步抢占，会产生概率性野值 bug；静态阅读代码很难发现，不属于语法编译报错**。
4. 调试阶段的脚手架代码，**不要直接留在交付业务代码中**；完整归档到文档，方便后续复盘复现历史 bug。

## 附录 A：本次修复前后关键变更 diff 摘要

1. ✔ 修复：IRQ 内部 static 状态变量 → 全局`volatile uint16_t`；
2. ✔ 新增：完整帧下标越界防御判断；
3. ✔ 加固：Serial_Printf `vsprintf` → `vsnprintf`缓冲区截断防护；
4. ✔ 清理：全部 dbg_*、快照、OLED 调试面板、DBG 打印全部移除；
5. ✔ 保留：业务告警 #后非！帧丢弃；全部业务 handler、协议、底层驱动零改动。


# 第三部分：log_record/TestCases.md 回归测试用例文档
```markdown
# 串口V3协议回归测试用例文档
> 路径：`log_record/TestCases.md`
> 用途：每次修改stm32下位机业务代码之后，执行全部用例，防止功能回归；实训/毕设测试报告可以直接摘抄。
> 硬件环境：stm32_f103_c8t6_slave；串口助手关闭本地回显；波特率9600；文本模式发送，不自动追加\r\n。

## 前置条件
1. 开发板硬件复位，上电；
2. OLED业务界面正常显示传感器数据；
3. 串口收到上电打印：`System Power Up, Ready!`；
4. 全部外设硬件接线确认无误。

## 测试用例表
|序号|测试分类|下发文本报文|预期输出（串口）|预期硬件现象|通过√ /失败×|备注|
|---|---|---|---|---|---|---|
|1|合法短帧‑LED开|`@LED:ON#!`|`Resp:LED ON OK#\r\n`|LED点亮| |基准用例|
|2|合法短帧‑LED关|`@LED:OFF#!`|`Resp:LED OFF OK#\r\n`|LED熄灭| |基准用例|
|3|合法‑蜂鸣器开|`@BEEP:ON#!`|`Resp:BEEP ON OK#\r\n`|蜂鸣器持续响| | |
|4|合法‑蜂鸣器关|`@BEEP:OFF#!`|`Resp:BEEP OFF OK#\r\n`|蜂鸣器关闭| | |
|5|合法‑舵机90°|`@SG90:90#!`|`Resp:SG90 set 90 OK#\r\n`|舵机转动到90度位置| |边界中间值|
|6|合法‑舵机0°|`@SG90:0#!`|`Resp:SG90 set 0 OK#\r\n`|舵机0°| |边界最小值|
|7|合法‑舵机180°|`@SG90:180#!`|`Resp:SG90 set 180 OK#\r\n`|舵机180°| |边界最大值|
|8|非法舵机参数超限|`@SG90:200#!`|`Resp:SG90 range err(0~180)#\r\n`|舵机保持原有角度| |范围校验测试|
|9|合法‑读取传感器|`@GET_DATA#!`|传感器组包上报`T:xx.x L:xxx AX:xxx … #\r\n`|无外设动作，OLED同步刷新传感器| | |
|10|合法‑设置高阈值30.5|`@SET_H:30.5#!`|`Resp:SET_H=30.5 OK#\r\n`|OLED界面H字段更新为30.5| |浮点参数|
|11|合法‑设置低阈值22.0|`@SET_L:22.0#!`|`Resp:SET_L=22.0 OK#\r\n`|OLED界面L字段更新22.0| |浮点参数|
|12|合法‑报警开启|`@ALARM:ON#!`|`Resp:ALARM ENABLE OK#\r\n`|OLED界面ALARM:ON| | |
|13|合法‑报警关闭|`@ALARM:OFF#!`|`Resp:ALARM DISABLE OK#\r\n`|OLED界面ALARM:OFF；声光报警强制关闭| | |
|14|非法帧：#后面不是! |`@ABC#X`|`[WARN] Frame discard: # followed by non-!\r\n`|无任何业务动作，LED/舵机不变| **业务告警测试**|协议异常输入|
|15|空合法载荷帧|`@#!`|`Resp:UNKNOWN CMD#\r\n`|无外设动作| |载荷为空，帧头帧尾合法，无匹配指令|
|16|超长报文（接近99字节）|构造载荷98字节，`@+98字符+#!`|帧丢弃，不执行业务；状态机回到IDLE|所有外设保持原有状态|缓冲区溢出边界测试|

## 回归测试执行步骤
1. 硬件复位开发板；
2. 按表格顺序逐条下发报文，每一条执行完成，记录结果；
3. 一旦出现失败，停止测试；优先回到`log_record/DebugRecord.md`查看历史故障排查流程；
4. 全部用例通过，代表串口协议模块回归测试通过。

## 异常现象参考对照表
|观测现象|优先排查方向|
|---|---|
|下发合法报文完全无应答|检查USART1硬件接线；波特率；帧头`@`、帧尾`#!`是否完整发送；确认没有半帧卡死；|
|打印乱码，但是外设动作偶尔可以执行|参考DebugRecord.md：中断状态变量static坑；`\0`终止符错位；|
|反复循环打印DISPATCH|业务handler内部阻塞卡死（I2C/硬件死等），函数无法return，执行流无法走到Serial_RxFlag清零；|
|出现`[WARN] Frame discard`|收到`#`后面跟随非`!`，属于协议非法报文，业务告警属于预期行为；|
|舵机/浮点阈值解析数值随机|优先确认`\0`终止符位置；其次排查sscanf MicroLib坑；|

### 一、解析器流控与防护

1. 流控模型：**A‑1，parser 模块内部`static bool frame_busy`忙标记 + `sensor_parser_release()`接口**
   - 契约：`sensor_parser_input_byte()`返回`1`，代表产出独立帧副本；**所有业务执行路径（正常、解析异常、IO 异常）都必须调用`sensor_parser_release()`**；
   - 实现策略：**L1，不使用 goto 统一出口**；依靠代码审查 + 专项单元测试保证全部分支走到 release；代码标记`/* @TODO 后续扩展：业务规模上涨，可评估goto统一出口 */`。
2. 忙状态下字节丢弃策略：**F1‑A**
   - `frame_busy==true`时，输入字节全部丢弃；**仅第一次进入 busy 状态打印一次告警**，后续持续丢弃不再刷屏；调用 release 恢复正常时打印恢复提示；F1‑B 每字节打印作为后续扩展。
3. 组帧缓冲区：`#define PARSE_WORK_BUF_LEN 128U`；
   - 恶意`$`开头、永远无`#`半帧：载荷达到 128 字节触发告警，清空`parse_work_buf`，parser 切回`IDLE`；**ring_buf 原始字节完整保留，下一轮轮询继续消费**。
4. 帧拷贝策略：`sensor_parser_input_byte()`内部收到`#`，将组帧缓存拷贝到输出参数`SensorFrame_t *out_frame`独立副本，业务层绝不触碰 parser 内部工作缓存。
5. ring_buf 消费策略：**S2‑A 单字节逐个喂入 parser**；S2‑B 批量解析标记`@TODO后续扩展`；ring_buf 大小 = 256 字节；ring_buf 写满丢弃新来字节，打印严重兜底告警。

### 二、CSV 日志 IO 错误策略（F2‑B，你的选择）

> 
> 业务前提：日志文件是本阶段核心业务依赖；**如果日志文件无法 open，说明基础运行环境损坏，后续运行无业务意义**。

- 当`open()`创建 / 打开 CSV 日志文件失败：打印 stderr 错误；完整释放全部已申请资源（串口 fd、各类句柄），进程`exit(EXIT_FAILURE)`直接退出；
- 后续运行过程中，`write()` / `fsync()`调用返回‑1（运行中磁盘异常）：打印 stderr 错误；**优先执行`sensor_parser_release()`释放 parser 忙标记（绝对不能漏，否则解析器锁死）**，再释放全部资源，进程退出；

> 
> ⚠️关键红线：**哪怕日志 IO 发生致命错误，在 exit 之前，必须先调用 release，防止 parser 处于 busy 卡死状态（虽然进程马上销毁内存，但是代码层面要严格遵守模块契约，单元测试也要覆盖该分支）**。

### 三、Linux 主循环与系统调用约束

1. poll 多路复用：poll 超时 50ms；同时监听`STDIN_FILENO`(控制台)、串口 fd；
2. 信号处理：捕获`SIGINT(Ctrl+C)`；触发之后执行完整资源释放，正常退出；
3. 串口`read()`返回‑1（USB 拔设备、IO 故障）：资源清理，进程退出；**USB 热插拔重连标记后续扩展**；
4. IO 硬性约束：**全部文件 IO 使用 POSIX 系统调用`open/read/write/lseek/fsync/close`；禁止`fopen/fprintf`等 C 库 FILE 接口；允许 memcpy/memset/strlen 内存处理函数**。

### 四、两套完全隔离通信协议

1. 📥 Linux → STM32 下发控制帧：`@载荷#!`
   - Linux 侧：控制台 stdin 读取一行，剥离`\r`、`\n`换行；原始字节直接 write 下发串口；**Linux 不解析下发指令语义，全部交给 STM32 下位机三段状态机解析**。
2. 📤 STM32 → Linux 传感器上报帧：`$T:25.1 L:1024 AX:120 AY:211 AZ:1630 GX:-11 GY:22 GZ:-33#\r\n`
   - STM32 修改`Uart_SendSensorFrame()`：新增`$`帧头，删除`#`前面多余空格；`\r\n`仅用于串口助手显示换行；
   - Linux 独立三段状态机解析上报帧；`\r\n`作为辅助字符直接丢弃。

### 五、三层防护架构（最终版，毕设论文 / 面试口述素材）

表格

| 防护层级 | 模块 | 解决风险 | 落地手段 |
| --- | --- | --- | --- |
| 第一层 协议解析层 | sensor_frame_parser | 1. 恶意`$`开头、永不发送`#`无限半帧；2. 完整帧收到未 release，防止组帧缓存被覆盖 | 128 字节 parse_work_buf 半帧溢出告警丢弃；内部`frame_busy`忙标记；busy=true 丢弃输入字节，仅首次告警；收到`#`拷贝独立帧副本输出；必须调用 release |
| 第二层 业务调度层 | main poll 主循环 + ring_buf | 区分 STM32 硬件抢占中断；Linux 用户态 parser 不会主动执行；硬件字节先蓄水池缓存 | 硬件字节全部进入 ring_buf；parser 只能被 main 轮询调用，无抢占；S2‑A 单字节喂解析器 |
| 第三层 底层硬件字节兜底 | ring_buf 256 字节 | 极端流量远超处理能力，前两层防护来不及消化 | ring_buf 写满，丢弃新来字节，打印严重告警；仅兜底，正常业务不会触发；后续扩展可增加溢出统计 |

### 六、全部`@TODO 后续扩展点`清单（全部写入代码注释 + 文档）

1. 解析器：S2‑B 批量字节输入，提升大数据流量性能；
2. 解析器：业务规模扩大之后，评估`goto`统一出口，彻底消除漏调用`sensor_parser_release()`人为风险；
3. 串口：USB 设备热插拔自动重连逻辑；
4. 控制台：简易友好命令层，输入`led_on`自动组装`@LED:ON#!`下发（当前版本原始帧透传）；
5. 协议：增加 CRC 校验；
6. ring_buf：溢出计数统计、故障复位策略；
7. parser：F1‑B 每丢弃字节打印调试日志（仅调试模式开启）。

### 七、重要契约（代码注释、文档、测试用例**加粗高亮**）

> 
> `sensor_parser_input_byte()`返回`1`，代表产出一份独立`SensorFrame_t`副本。
> ✅**无论：业务正常完成 / KV 字段解析出错 / CSV write/fsync 磁盘 IO 报错，该执行路径在离开本帧处理逻辑之前，必须调用一次`sensor_parser_release()`**。
> ❗风险：一旦漏调用 release，`frame_busy`永久 true；解析器会全部丢弃后续所有上报帧，永久失效。
> 🧪专项测试用例：必须覆盖「IO 错误分支，验证即使写 CSV 失败，依然调用 release」，防止回归。
