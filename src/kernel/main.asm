org 0x0                 ; BIOS loads boot sector to 0x7C00
bits 16                 ; 16 bit code

; New line macro
%define ENDL 0x0D, 0x0A  

; Boot sector code goes here
start:
    mov si, msg_hello
    call puts

.halt:
    cli
    hlt

;
;   Print a string to the screen
;   Parameters:
;       ds:si - pointer to the string
;
puts:
    push si         ; Save si
    push ax         ; Save ax
    puah bx         

.loop:
    lodsb           ; Load next byte dl:si into al register and increment si
    or al, al       ; Check if al is 0 (end of string). Does not change the value of al but sets 
                    ; the flags according to the result

    jz .done        ; If al is 0, we're done

    mov ah, 0x0E    ; BIOS teletype function
    mov bh, 0       ; Page number
    int 0x10        ; Call BIOS

    jmp .loop        ; And repeat

.done:
    pop bx
    pop ax          ; Restore ax
    pop si          ; Restore si
    ret             ; Return to the caller

msg_hello: db 'Hello World!', ENDL, 0
