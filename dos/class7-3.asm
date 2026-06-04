; 实验2：8255方式1输入口程序
.MODEL SMALL
.DATA
    PORT_A      EQU 288H    ; 8255 PA口地址
    PORT_CTRL   EQU 28BH    ; 8255 控制口地址
    OLD_INT_SEG DW ?
    OLD_INT_OFF DW ?
    INT_COUNT   DW 8        ; 中断次数计数器，初始为8

.CODE
START:
    MOV AX, @DATA
    MOV DS, AX

    ; 1. 保存原IRQ3中断向量
    MOV AX, 350BH
    INT 21H
    MOV OLD_INT_OFF, BX
    MOV OLD_INT_SEG, ES

    ; 2. 设置新的IRQ3中断向量
    PUSH DS
    MOV AX, SEG IN_ISR
    MOV DS, AX
    MOV DX, OFFSET IN_ISR
    MOV AX, 250BH
    INT 21H
    POP DS

    ; 3. 解除8259对IRQ3的屏蔽
    IN AL, 21H
    AND AL, 0F7H
    OUT 21H, AL

    ; 4. 初始化8255：A口方式1输入
    ; 控制字：1(特征位) 01(A口方式1) 1(A口输入) 0(C上半部输出) 0(B口方式0) 0(B口输出) 0(C下半部输出)
    MOV DX, PORT_CTRL
    MOV AL, 0B0H            
    OUT DX, AL

    ; 5. 使能8255的INTE A (对于A口输入，由PC4置位实现)
    ; 置位/复位控制字：0(特征位) 000 100(选择PC4) 1(置位) = 09H
    MOV AL, 09H             
    OUT DX, AL

    ; 6. 等待中断循环
WAIT_LOOP:
    CMP INT_COUNT, 0        ; 判断计数器CX是否为0 (在中断中递减)
    JNZ WAIT_LOOP

    ; 7. 恢复系统环境并退出
    LDS DX, DWORD PTR OLD_INT_OFF
    MOV AX, 250BH
    INT 21H

    IN AL, 21H
    OR AL, 08H
    OUT 21H, AL

    MOV AH, 4CH
    INT 21H

; --- 中断服务程序 ---
IN_ISR PROC FAR
    PUSH AX
    PUSH DX

    ; 从A口读取外部开关预置的ASCII码数据
    MOV DX, PORT_A
    IN AL, DX

    ; 在屏幕上显示该字符 (DOS 02H 功能调用)
    MOV DL, AL
    MOV AH, 02H
    INT 21H

    ; 中断次数计数器减1
    DEC INT_COUNT           

    ; 向8259发送中断结束命令(EOI)
    MOV AL, 20H
    OUT 20H, AL

    POP DX
    POP AX
    STI                     ; 开中断
    IRET                    ; 中断返回
IN_ISR ENDP

END START
