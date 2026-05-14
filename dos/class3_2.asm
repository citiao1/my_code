DATAS SEGMENT
    PROMPT   DB 'Please input a string with *: $'
    RES_MSG  DB 0Dh, 0Ah, 'After deleted *: $'

    MAX_LEN  DB 100
    ACT_LEN  DB ?
    BUFFER   DB 100 DUP(0)
DATAS ENDS

CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS
START:
    MOV AX, DATAS
    MOV DS, AX

    LEA DX, PROMPT
    MOV AH, 09H
    INT 21H

    LEA DX, MAX_LEN
    MOV AH, 0AH
    INT 21H

    LEA DX, RES_MSG
    MOV AH, 09H
    INT 21H

    MOV CL, ACT_LEN
    MOV CH, 0
    JCXZ EXIT_PROG      
    
    LEA BX, BUFFER       

PRINT_LOOP:
    MOV DL, [BX]         
    CMP DL, '*'          
    JE SKIP_PRINT        

    MOV AH, 02H
    INT 21H

SKIP_PRINT:
    INC BX               
    LOOP PRINT_LOOP     

EXIT_PROG:
    MOV AH, 4CH
    INT 21H
CODES ENDS
END START

