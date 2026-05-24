data segment
io8255a  equ 288h
io8255c  equ 28ch
io8255cr equ 28eh
; 共阴极段码表 0~9
led db 3fh, 06h, 5bh, 4fh, 66h, 6dh, 7dh, 07h, 7fh, 6fh

; ---> 请在此处填入你学号的最后4位数字 <---
stu_id db 1, 2, 3, 4 

; 4位数码管的位选码表，依次选中 S0, S1, S2, S3 (对应 PC0~PC3)
sel_code db 01h, 02h, 04h, 08h 
data ends

code segment
assume cs: code, ds: data
start:
    mov ax, data
    mov ds, ax

    ; 初始化8255为方式0全输出
    mov dx, io8255cr
    mov al, 80h   
    out dx, al

scan_loop:
    mov cx, 4        ; 4位数码管，循环扫描4次
    mov si, 0        ; 学号与位选码的索引

disp_next:
    ; 1. 查表获取当前数字的段码
    mov bl, stu_id[si] ; 获取当前要显示的学号数字
    mov bh, 0
    mov al, led[bx]    ; 根据数字查出对应的段码

    ; 2. 送数码管段码到A口
    mov dx, io8255a
    out dx, al

    ; 3. 送位码到C口 (点亮对应的数码管)
    mov al, sel_code[si]
    mov dx, io8255c
    out dx, al

    ; 4. 延时 (动态扫描的延时需要很短，以利用人眼视觉暂留效应)
    call scan_delay

    ; 5. 位码全为0，关闭所有数码管 (消影，防止数字重叠)
    mov al, 00h
    mov dx, io8255c
    out dx, al

    ; 准备显示下一位
    inc si
    loop disp_next

    ; 检查是否有键按下以退出程序
    mov ah, 6
    mov dl, 0ffh
    int 21h
    jnz exit         ; 有键按下则退出
    jmp scan_loop    ; 无键按下，重新开始新一轮的4位扫描

exit:
    mov ah, 4ch
    int 21h

; 动态扫描专用短延时子程序
scan_delay proc
    push cx
    mov cx, 0500h    ; 延时时间较短，避免闪烁且保证亮度
scan_d1: 
    nop
    loop scan_d1
    pop cx
    ret
scan_delay endp

code ends
end start
