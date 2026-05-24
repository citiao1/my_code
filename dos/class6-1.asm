data segment
io8255a  equ 288h  ; 8255 A口地址
io8255c  equ 28ch  ; 8255 C口地址 (已修正原书错误)
io8255cr equ 28eh  ; 8255 控制寄存器地址 (已修正原书错误)
; 共阴极七段数码管段码表 0~9
led db 3fh, 06h, 5bh, 4fh, 66h, 6dh, 7dh, 07h, 7fh, 6fh 
data ends

code segment
assume cs: code, ds: data
start:
    mov ax, data
    mov ds, ax

    ; 1. 寄存器初始化与设置8255工作方式0
    mov dx, io8255cr 
    mov al, 80h      ; 控制字80H：方式0，A口输出，B口输出，C口输出
    out dx, al

    ; 2. 静态显示位选：位码输入端S0接PC0，输出1选中最低位数码管
    mov dx, io8255c
    mov al, 01h
    out dx, al

leddisp: 
    mov bx, offset led 
    mov cx, 10       ; 循环10次 (显示0~9) [cite: 288]
    mov si, 0        ; 设置查表指针偏移量

disp_loop:
    ; 3. 查表输出字符段码到A口
    mov al, [bx+si]
    mov dx, io8255a
    out dx, al

    ; 4. 调用软件延时
    call delay

    ; 修改指针
    inc si

    ; 5. 检查是否有键按下
    mov ah, 6        ; DOS调用，直接控制台I/O [cite: 292]
    mov dl, 0ffh     ; DL=FFH 表示读取键盘缓冲区 [cite: 293]
    int 21h          ; [cite: 294]
    jnz exit         ; 如果有按键(ZF=0标志清除)，跳转结束程序

    loop disp_loop   ; 表格未到末尾，继续循环显示下一个数字

    jmp leddisp      ; 10个数字显示完毕，指针重新指向表头 [cite: 258]

exit:
    mov ah, 4ch      ; [cite: 296]
    int 21h          ; [cite: 297]

; 软件延时子程序
delay proc
    push cx
    mov cx, 0FFFFh   ; 设置较长的延时，以便肉眼观察数字变化
d1: nop
    nop
    loop d1
    pop cx
    ret
delay endp

code ends
end start 
