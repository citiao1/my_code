DATAS SEGMENT
    PROMPT   DB 'Please input a string: $'
    RES_MSG  DB 0Dh, 0Ah, 'Digit count is: $'
    
    ; 0AH 号功能专属的数据缓冲区格式：
    MAX_LEN  DB 100          ; 第1个字节：允许输入的最大长度
    ACT_LEN  DB ?            ; 第2个字节：DOS 自动填入的实际输入长度
    BUFFER   DB 100 DUP(0)   ; 从第3个字节开始：存放真正输入的字符
DATAS ENDS

STACKS SEGMENT
    DB 100H DUP(0)
STACKS ENDS

CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS,SS:STACKS
START:
    MOV AX, DATAS
    MOV DS, AX

    ; 1. 打印提示信息
    LEA DX, PROMPT
    MOV AH, 09H
    INT 21H

    ; 2. 接收字符串输入
    LEA DX, MAX_LEN
    MOV AH, 0AH
    INT 21H

    ; 3. 准备遍历统计
    MOV CL, ACT_LEN      ; 把实际输入的字符数量放进 CX 作为循环次数
    MOV CH, 0            ; 高位清零，保证 CX 的值就是实际长度
    JCXZ FINISH          ; 如果没输入任何字符 (CX=0)，直接跳到结束
    
    LEA BX, BUFFER       ; BX 指向字符串真正的开头
    MOV DL, 0            ; 用 DL 作为数字字符的计数器，初始为 0

COUNT_LOOP:
    MOV AL, [BX]         ; 取出一个字符放到 AL
    CMP AL, '0'
    JB NEXT_CHAR         ; 小于 '0'，不是数字，跳过
    CMP AL, '9'
    JA NEXT_CHAR         ; 大于 '9'，不是数字，跳过
    INC DL               ; 走到这里说明是数字，计数器 +1

NEXT_CHAR:
    INC BX               ; BX 指向下一个字符
    LOOP COUNT_LOOP      ; CX 减 1，如果不为 0 则跳回 COUNT_LOOP 继续

FINISH:
    ; 4. 打印结果提示
    PUSH DX              ; 暂存计数器 DL 的值
    LEA DX, RES_MSG
    MOV AH, 09H
    INT 21H
    POP DX               ; 恢复计数器 DL 的值

    ; 5. 把 DL 里的个数打印出来 (除以 10 拆分十位和个位)
    MOV AL, DL
    MOV AH, 0
    MOV BL, 10
    DIV BL               ; AX / 10，商(十位)在AL，余数(个位)在AH
    MOV BX, AX           ; 暂存结果到 BX
    
    MOV DL, BL           ; 打印十位
    ADD DL, 30H
    MOV AH, 02H
    INT 21H
    
    MOV DL, BH           ; 打印个位
    ADD DL, 30H
    MOV AH, 02H
    INT 21H

    ; 退出程序
    MOV AH, 4CH
    INT 21H
CODES ENDS
END START
