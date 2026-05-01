DATAS SEGMENT
    MSG_DIGIT DB 0Dh, 0Ah, 'DIGIT', 0Dh, 0Ah, '$'
    MSG_CHAR  DB 0Dh, 0Ah, 'CHAR', 0Dh, 0Ah, '$'
    MSG_OTHER DB 0Dh, 0Ah, 'OTHER', 0Dh, 0Ah, '$'
DATAS ENDS



CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS
START:
    MOV AX, DATAS
    MOV DS, AX


    MOV AH, 01H
    INT 21H             


    CMP AL, '0'
    JB CHECK_UPPER      
    CMP AL, '9'
    JA CHECK_UPPER     
    LEA DX, MSG_DIGIT  
    JMP PRINT_RES

CHECK_UPPER:

    CMP AL, 'A'
    JB IS_OTHER        
    CMP AL, 'Z'
    JBE IS_CHAR         

CHECK_LOWER:

    CMP AL, 'a'
    JB IS_OTHER        
    CMP AL, 'z'
    JA IS_OTHER        

IS_CHAR:

    LEA DX, MSG_CHAR
    JMP PRINT_RES

IS_OTHER:

    LEA DX, MSG_OTHER

PRINT_RES:
    MOV AH, 09H
    INT 21H

    MOV AH, 4CH
    INT 21H
CODES ENDS
END START

