DATAS SEGMENT
    ; 定义共阴极段码表，按照 0~9 的顺序排列
    SEG_TABLE DB 3FH, 06H, 5BH, 4FH, 66H, 6DH, 7DH, 07H, 7FH, 6FH
    TEST_NUM  DB 7    ; 这里假设我们要查数字 7 的段码，你可以随意修改(0~9)
DATAS ENDS

STACKS SEGMENT
    db 100h dup(0)
STACKS ENDS

CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS,SS:STACKS
START:
    ; 初始化数据段
    MOV AX, DATAS
    MOV DS, AX

    ; --- 实验 1 核心代码 ---
    MOV AL, TEST_NUM  ; AL 中放入要查找的数字
    LEA BX, SEG_TABLE ; 将 BX 指向段码表的首地址 (表头)
    XLAT              ; 执行查表：AL = DS:[BX + AL]
    MOV DL, AL        ; 按照题目要求，将查到的段码值送到 DL 寄存器

    ; 退出程序
    MOV AH, 4CH
    INT 21H
CODES ENDS
END START