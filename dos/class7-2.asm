; 实验1：8255方式1输出口程序
.MODEL SMALL
.DATA
    PORT_A      EQU 288H    ; 8255 PA口地址
    PORT_CTRL   EQU 28BH    ; 8255 控制口地址
    OLD_INT_SEG DW ?        ; 保存原中断向量段地址
    OLD_INT_OFF DW ?        ; 保存原中断向量偏移地址
    DATA_VAL    DB 01H      ; 初始输出数据，设为01H

.CODE
START:
    MOV AX, @DATA
    MOV DS, AX

    ; 1. 保存原IRQ3 (INT 0BH) 的中断向量
    MOV AX, 350BH
    INT 21H
    MOV OLD_INT_OFF, BX
    MOV OLD_INT_SEG, ES

    ; 2. 设置新的IRQ3中断向量，指向自定义的中断服务程序
    PUSH DS
    MOV AX, SEG OUT_ISR
    MOV DS, AX
    MOV DX, OFFSET OUT_ISR
    MOV AX, 250BH
    INT 21H
    POP DS

    ; 3. 解除8259对IRQ3的屏蔽 (修改IMR寄存器)
    IN AL, 21H
    AND AL, 0F7H            ; 将位3清零，允许IRQ3中断
    OUT 21H, AL

    ; 4. 初始化8255：A口方式1输出
    ; 控制字：1(特征位) 01(A口方式1) 0(A口输出) 0(C上半部输出) 0(B口方式0) 0(B口输出) 0(C下半部输出)
    MOV DX, PORT_CTRL
    MOV AL, 0A0H            
    OUT DX, AL

    ; 5. 使能8255的INTE A (对于A口输出，由PC6置位实现)
    ; 置位/复位控制字：0(特征位) 000 110(选择PC6) 1(置位) = 0DH
    MOV AL, 0DH             
    OUT DX, AL

    ; 6. 等待中断循环
WAIT_LOOP:
    CMP DATA_VAL, 0         ; 判断DATA_VAL是否移位至0 (8次中断后01H左移8次变0)
    JNZ WAIT_LOOP           ; 未结束则继续等待

    ; 7. 恢复原中断向量和环境并退出程序
    LDS DX, DWORD PTR OLD_INT_OFF
    MOV AX, 250BH
    INT 21H

    IN AL, 21H              ; 恢复IRQ3屏蔽
    OR AL, 08H
    OUT 21H, AL

    MOV AH, 4CH             ; 返回DOS
    INT 21H

; --- 中断服务程序 ---
OUT_ISR PROC FAR
    PUSH AX
    PUSH DX

    ; 将数据从A口输出到LED
    MOV DX, PORT_A
    MOV AL, DATA_VAL
    OUT DX, AL

    ; 数据左移一位，为下一次中断准备 (01H -> 02H -> 04H... -> 80H -> 00H)
    SHL DATA_VAL, 1         

    ; 向8259发送中断结束命令(EOI)
    MOV AL, 20H
    OUT 20H, AL

    POP DX
    POP AX
    STI                     ; 开中断
    IRET                    ; 中断返回
OUT_ISR ENDP

END START
