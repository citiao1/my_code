DATAS SEGMENT
    PROMPT   DB 'Please input a string: $'
    MSG_LEN  DB 0Dh, 0Ah, 'Total Length: $'
    MSG_DIG  DB 0Dh, 0Ah, 'Digit Count : $'
    MSG_ALP  DB 0Dh, 0Ah, 'Alpha Count : $'
    MSG_OTH  DB 0Dh, 0Ah, 'Other Count : $'


    CNT_DIG  DB 0
    CNT_ALP  DB 0
    CNT_OTH  DB 0
    

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

    MOV CL, ACT_LEN    
    MOV CH, 0
    JCXZ PRINT_ALL       

    LEA BX, BUFFER       

CHECK_LOOP:
    MOV AL, [BX]         
    
    CMP AL, '0'
    JB IS_OTHER         
    CMP AL, '9'
    JBE IS_DIGIT        

    
    CMP AL, 'A'
    JB IS_OTHER          
    CMP AL, 'Z'
    JBE IS_ALPHA        

    CMP AL, 'a'
    JB IS_OTHER          
    CMP AL, 'z'
    JBE IS_ALPHA         

IS_OTHER:
    INC CNT_OTH          
    JMP NEXT_CHAR       

IS_DIGIT:
    INC CNT_DIG          
    JMP NEXT_CHAR

IS_ALPHA:
    INC CNT_ALP          
    JMP NEXT_CHAR

NEXT_CHAR:
    INC BX               
    LOOP CHECK_LOOP      

    
PRINT_ALL:
   
    LEA DX, MSG_LEN
    MOV AH, 09H
    INT 21H
    MOV AL, ACT_LEN      
    CALL PRINT_NUM      


    LEA DX, MSG_DIG
    MOV AH, 09H
    INT 21H
    MOV AL, CNT_DIG
    CALL PRINT_NUM

    LEA DX, MSG_ALP
    MOV AH, 09H
    INT 21H
    MOV AL, CNT_ALP
    CALL PRINT_NUM

    LEA DX, MSG_OTH
    MOV AH, 09H
    INT 21H
    MOV AL, CNT_OTH
    CALL PRINT_NUM

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

