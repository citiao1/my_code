; -------- 修改这里 --------
outport1 equ 3a0h        ; 将点亮端口地址改为 3A0H
outport2 equ 3b0h        ; 将熄灭端口地址改为 3B0H
; --------------------------

code segment
    assume cs: code

start:  
    mov dx, outport1
    out dx, al           ; 访问3A0H，Y4输出负脉冲，L7点亮
    call delay           

    mov dx, outport2
    out dx, al           ; 访问3B0H，Y6输出负脉冲，L7熄灭
    call delay           

    jmp start            

    mov ah, 4ch          
    int 21h              

delay proc near          
    mov bx, 20           
zz: mov cx, 2000         
z:  loop z               
    dec bx               
    jne zz               
    ret                  
delay endp               

code ends                
    end start
