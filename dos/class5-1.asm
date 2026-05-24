outport1 equ 2a0h        ; 定义Y4输出对应的端口地址，用于点亮L7 [cite: 313]
outport2 equ 2a8h        ; 定义Y5输出对应的端口地址，用于熄灭L7 [cite: 314]

code segment
    assume cs: code

start:  
    mov dx, outport1
    out dx, al           ; Y4输出负脉冲，D触发器CLK有效，L7点亮 [cite: 320, 332]
    call delay           ; 调用延时子程序，保持亮的状态 [cite: 333]

    mov dx, outport2
    out dx, al           ; Y5输出负脉冲，D触发器R有效，L7熄灭 [cite: 334]
    call delay           ; 调用延时子程序，保持灭的状态 [cite: 335]

    jmp start            ; 无条件跳转回start，形成亮灭闪烁的死循环 [cite: 321]

    ; 规范退出程序的代码（虽然当前是死循环执行不到，但建议保留作为规范）
    mov ah, 4ch          ; [cite: 322]
    int 21h              ; [cite: 323]

delay proc near          ; 延时子程序 [cite: 324]
    mov bx, 20           ; [cite: 325]
zz: mov cx, 2000         ; [cite: 326]
z:  loop z               ; [cite: 327]
    dec bx               ; [cite: 328]
    jne zz               ; [cite: 329]
    ret                  ; [cite: 330]
delay endp               ; [cite: 331]

code ends                ; [cite: 338]
    end start            ; [cite: 339]
