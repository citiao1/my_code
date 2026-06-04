; ==========================================
; 8259中断控制器与8255接口 Proteus仿真实验代码
; ==========================================

; 8255 端口地址 (注：8255的A0,A1接CPU的A1,A2，因此地址偏移为0, 2, 4, 6)
PA8255  EQU 288H    ; PA 口地址 (Base + 0)
PC8255  EQU 28CH    ; PC 口地址 (Base + 4)
CTL8255 EQU 28EH    ; 控制字寄存器地址 (Base + 6)

; 8259 命令字
ICW1    EQU 13H     ; 单片8259(D1=1)，边沿触发(D3=0)，要写ICW4(D0=1)
ICW2    EQU 60H     ; IRQ3中断类型号为63H，推导出IRQ0基址为60H
ICW4    EQU 01H     ; 工作在8086方式(D0=1)，中断非自动结束(D1=0)
OCW1    EQU 00H     ; 开放所有中断源 (若只开放IRQ3可写为 F7H)

P8259   EQU 2A0H    ; 8259 的偶地址 (接CPU A1，所以是A0H)
O8259   EQU 2A2H    ; 8259 的奇地址 (A2H)

DATA SEGMENT
    ; 共阴极数码管 0-9 的段码表
    TABLE DB 3FH, 06H, 5BH, 4FH, 66H, 6DH, 7DH, 07H, 7FH, 6FH 
DATA ENDS

CODE SEGMENT
    ASSUME CS: CODE, DS: DATA

START:
    MOV AX, DATA
    MOV DS, AX

    ; --- 1. 8255 初始化 ---
    MOV DX, CTL8255
    MOV AL, 80H     ; 控制字80H：方式0，PA、PB、PC均设为输出
    OUT DX, AL

    CLI             ; 关中断，准备修改中断向量表

    ; --- 2. 设置 63H 号的中断向量 ---
    MOV AX, 0
    MOV ES, AX
    MOV SI, 63H*4   ; 63H乘4得到中断向量表中的物理偏移地址
    MOV AX, OFFSET INT3
    MOV ES:[SI], AX ; 存入中断服务程序的偏移地址 (IP)
    MOV AX, SEG INT3
    MOV ES:[SI+2], AX ; 存入中断服务程序的段地址 (CS)

    ; --- 3. 8259 初始化 ---
    MOV DX, P8259
    MOV AL, ICW1
    OUT DX, AL      ; 写入偶地址：ICW1
    
    MOV DX, O8259
    MOV AL, ICW2
    OUT DX, AL      ; 写入奇地址：ICW2
    
    MOV AL, ICW4
    OUT DX, AL      ; 写入奇地址：ICW4
    
    MOV AL, OCW1
    OUT DX, AL      ; 写入奇地址：OCW1 (中断屏蔽寄存器)

    ; --- 4. 显示初始化配置 ---
    MOV SI, OFFSET TABLE
    MOV CX, 10      ; 设置循环次数（10次）
    
    MOV DX, PC8255
    MOV AL, 1
    OUT DX, AL      ; PC0输出1，选通数码管（经过原理图中的NOT反相器后变低电平有效）
    
    MOV DX, PA8255
    MOV AL, [SI]
    OUT DX, AL      ; PA口输出段码，初始显示 '0'

    STI             ; 开中断

    ; --- 5. 主循环等待中断 ---
    ; 教材特别标注：由于Proteus中8086模型的Bug，需要不断向总线发中断号
LP: MOV DX, P8259
    MOV AL, 63H
    OUT DX, AL
    JMP LP


; ==========================================
; 中断服务程序 INT3
; ==========================================
INT3:
    CLI             ; 进入中断后关中断
    MOV DX, PA8255
    INC SI          ; 指向查表中的下一个段码
    MOV AL, [SI]
    OUT DX, AL      ; 输出当前计数值到数码管
    
    DEC CX          ; 计数器减1
    JNZ NEXT        ; 如果未减到0（即没到10次），跳过重置步骤
    
    ; --- 10次中断后，重置计数器和显示 ---
    MOV CX, 10
    MOV SI, OFFSET TABLE
    MOV AL, [SI]
    OUT DX, AL      ; 重新显示“0”

NEXT:
    MOV DX, P8259
    MOV AL, 20H
    OUT DX, AL      ; 向8259发送普通中断结束命令 (EOI)
    
    STI             ; 恢复开中断
    IRET            ; 从中断返回

CODE ENDS
END START
