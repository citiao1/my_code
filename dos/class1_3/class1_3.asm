
CODES SEGMENT
    ASSUME CS:CODES
START:

    MOV AL, 53H 
    MOV AH, 0         
    MOV BL, 10        
    DIV BL           

    MOV BH, AH       


    MOV DL, AL      
    ADD DL, 30H     
    MOV AH, 02H      
    INT 21H

    MOV DL, BH       
    ADD DL, 30H      
    MOV AH, 02H
    INT 21H

    MOV AH, 4CH
    INT 21H
CODES ENDS
END START


