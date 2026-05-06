DATAS SEGMENT

    BUF DB 32H, 90H, 54H, 00H, 0F8H, 87H, 37H, 89H, 28H, 0F1H
    LEN EQU $ - BUF     
DATAS ENDS


CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS
START:
    MOV AX, DATAS
    MOV DS, AX


    MOV CX, LEN
    DEC CX            
    
OUTER_LOOP:
    MOV DX, CX          
    LEA BX, BUF        
    
INNER_LOOP:
    MOV AL, [BX]       
    CMP AL, [BX+1]     
    JBE NO_SWAP        
    

    XCHG AL, [BX+1]     
    MOV [BX], AL        

NO_SWAP:
    INC BX              
    LOOP INNER_LOOP     

    MOV CX, DX          
    LOOP OUTER_LOOP     

    MOV AH, 4CH
    INT 21H
CODES ENDS
END START



