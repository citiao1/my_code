DATAS SEGMENT
    SCORE_VAL DB 60
    MSG_EXC   DB 'Excellent', 0Dh, 0Ah, '$'
    MSG_MED   DB 'Medium level', 0Dh, 0Ah, '$'
    MSG_PASS  DB 'Pass', 0Dh, 0Ah, '$'
    MSG_FAIL  DB 'Failure', 0Dh, 0Ah, '$'
DATAS ENDS

CODES SEGMENT
    ASSUME CS:CODES,DS:DATAS
START:
    MOV AX, DATAS
    MOV DS, AX
    MOV AL, SCORE_VAL   
    CMP AL, 85          
    JAE IS_EXC          
    CMP AL, 70          
    JAE IS_MED          
    CMP AL, 60          
    JAE IS_PASS         
    LEA DX, MSG_FAIL
    JMP PRINT_RES

IS_EXC:
    LEA DX, MSG_EXC
    JMP PRINT_RES
IS_MED:
    LEA DX, MSG_MED
    JMP PRINT_RES
IS_PASS:
    LEA DX, MSG_PASS

PRINT_RES:
    MOV AH, 09H
    INT 21H

   
    MOV AH, 4CH
    INT 21H
CODES ENDS
END START


