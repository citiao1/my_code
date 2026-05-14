DATAS SEGMENT
    SCORES   DB 95, 67, 86, 75, 100, 52, 72, 80, 93, 87
    LEN      EQU $ - SCORES

    MSG_AVG  DB 'Average Score: $'
    MSG_EXC  DB 0Dh, 0Ah, 'Excellent Count(>=85): $'
    MSG_BEL  DB 0Dh, 0Ah, 'Below Average Count: $'

    AVG_VAL  DB 0
    EXC_CNT  DB 0
    BEL_CNT  DB 0
DATAS ENDS

STACKS SEGMENT
    DB 100H DUP(0)
STACKS ENDS

CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS,SS:STACKS
START:
    MOV AX, DATAS
    MOV DS, AX


    LEA BX, SCORES
    MOV CX, LEN
    MOV AX, 0           
SUM_LOOP:
    MOV DL, [BX]        
    MOV DH, 0           
    ADD AX, DX          
    INC BX
    LOOP SUM_LOOP

    MOV BL, 10
    DIV BL              
    MOV AVG_VAL, AL    


    LEA BX, SCORES
    MOV CX, LEN
COUNT_LOOP:
    MOV AL, [BX]

    
    CMP AL, 85
    JB CHECK_BELOW     
    INC EXC_CNT        

CHECK_BELOW:

    CMP AL, AVG_VAL
    JAE NEXT_SCORE      
    INC BEL_CNT         

NEXT_SCORE:
    INC BX
    LOOP COUNT_LOOP


    LEA DX, MSG_AVG
    MOV AH, 09H
    INT 21H
    MOV AL, AVG_VAL
    CALL PRINT_NUM

    LEA DX, MSG_EXC
    MOV AH, 09H
    INT 21H
    MOV AL, EXC_CNT
    CALL PRINT_NUM

    LEA DX, MSG_BEL
    MOV AH, 09H
    INT 21H
    MOV AL, BEL_CNT
    CALL PRINT_NUM

    ; ÍË³ö³ÌÐò
    MOV AH, 4CH
    INT 21H


PRINT_NUM PROC
    MOV AH, 0
    MOV BL, 10
    DIV BL      
    MOV BH, AH  

    MOV AH, 0
    DIV BL      
    MOV CX, AX  

    MOV DL, CL
    ADD DL, 30H 
    MOV AH, 02H
    INT 21H

    MOV DL, CH
    ADD DL, 30H
    MOV AH, 02H
    INT 21H

    MOV DL, BH
    ADD DL, 30H
    MOV AH, 02H
    INT 21H
    RET
PRINT_NUM ENDP

CODES ENDS
END START


