.8086
.model small
.stack 100h

TIMER_RATE      equ 8000h   ; double the rate of timer interrupts to 36.4/s

dseg segment word public
    assume ds:dseg

prevGfxMode     db 0
originalKBInt   dd 0
originalTmrInt  dd 0
kbState         db 0
hasBegun        db 0
xPos            dw 50
yPos            dw 50
xDir            db 1
yDir            db 1
canDrawNext     db 0
timerIntCt      db 0
screenArray     db 64000 dup(0) ; just about get away with this fitting into a segment

dseg ends

cseg segment word public
    assume ds:dseg,cs:cseg

timerInterrupt proc
    push ax
    mov canDrawNext, 1      ; timer interrupt has happened so next frame can be drawn

    mov al, timerIntCt      ; check if we should call the BIOS interrupt
    inc al                  ; (needed to avoid freezing the CPU clock)
    cmp al, 2               ; we run at 2x speed, so every 2nd call updates BIOS
    jge doBIOSTimer
    
    mov al, 20h             ; tell BIOS we handled the interrupt
    out 20h, al
    
    pop ax
    iret                    ; return without incrementing BIOS time

doBIOSTimer:
    pop ax
    jmp [originalTmrInt]    ; call the original timer interrupt
timerInterrupt endp

kbInterrupt proc
    push ds             ; save existing ds/ax values
    push ax
    
    mov ax, seg dseg    ; set correct data segment
    mov ds, ax

    in al, 60h          ; read from port 0x60
    xor ax, ax

    mov al, hasBegun    ; check we've done at least some iterations
    test al, al
    jz skipKBState

    mov al, kbState     ; add 1 to kbState
    inc al
    mov kbState, al

skipKBState:
    mov al, 20h         ; tell BIOS we handled the interrupt
    out 20h, al

    pop ax              ; restore ds/ax
    pop ds
    iret
kbInterrupt endp

setupInterrupts proc
    cli      
    push ds
    mov ax, 3509h                       ; AH = 35h (get interrupt vector), AL = 09h (keyboard)
    int 21h                             ; DOS interrupt
    mov word ptr originalKBInt, bx      ; store original KB interrupt location
    mov word ptr originalKBInt+2, es

    push cs                             ; put the code segment into DS
    pop ds
    assume ds:cseg

    lea dx, kbInterrupt         
    mov ax, 2509h                       ; AH = 25h (set interrupt vector), AL = 09h (keyboard)
    int 21h                             ; DOS interrupt

    pop ds                              ; restore ds for saving timer int handler
    assume ds:dseg
    push ds

    mov ax, 3508h                       ; AH = 35h (get interrupt vector), AL = 08h (timer)
    int 21h                             ; DOS interrupt
    mov word ptr originalTmrInt, bx     ; store original timer interrupt location
    mov word ptr originalTmrInt+2, es

    push cs                             ; put the code segment into DS
    pop ds
    assume ds:cseg

    lea dx, timerInterrupt         
    mov ax, 2508h                       ; AH = 25h (set interrupt vector), AL = 08h (timer)
    int 21h                             ; DOS interrupt

    pop ds
    assume ds:dseg

    mov al, 00110110b                   ; timer bits: 00 | 11 | 011 | 0
                                        ;             |    |    |     |
                                        ;             |    |    |     |- binary counter
                                        ;             |    |    |- square wave
                                        ;             |    |- write LSB followed by MSB
                                        ;             |- counter 0
    out 43h, al                         ; 43h = timer control port
    jmp $+2                             ; tick the CPU so the timer picks up changes
    mov cx, TIMER_RATE
    mov al, cl                          ; send LSB
    out 40h, al
    jmp $+2
    mov al, ch                          ; send MSB
    mov al, ch
    out 40h, al
    jmp $+2

    sti                                 ; set interrupt flag
    ret
setupInterrupts endp

restoreInterrupts proc
    cli
    push ds
    lds dx, originalKBInt   ; restore original kb interrupt adddress
    mov ax, 2509h           ; AH = 25h (set interrupt vector), AL = 09h (keyboard)
    int 21h                 ; DOS interrupt

    pop ds                  ; restore data segment to load the next address
    push ds

    mov al, 00110110b       ; timer bits: 00 | 11 | 011 | 0
                            ;             |    |    |     |
                            ;             |    |    |     |- binary counter
                            ;             |    |    |- square wave
                            ;             |    |- write LSB followed by MSB
                            ;             |- counter 0
    out 43h, al             ; 43h = timer control port
    jmp $+2                 ; tick the CPU so the timer picks up changes
    mov cx, 0               ; restore the original timer value
    mov al, cl              ; send LSB
    out 40h, al
    jmp $+2
    mov al, ch              ; send MSB
    mov al, ch
    out 40h, al
    jmp $+2

    lds dx, originalTmrInt  ; restore original timer interrupt adddress
    mov ax, 2508h           ; AH = 25h (set interrupt vector), AL = 08h (timer)
    int 21h                 ; DOS interrupt

    pop ds
    sti
    ret
restoreInterrupts endp

moveBox proc
    mov ax, xPos            ; check if we are at the right-hand boundary
    cmp ax, 280d
    jl notAtRightBoundary
    xor al, al              ; set xdir to 0 if at boundary
    mov xDir, al
    jmp checkVertical

notAtRightBoundary:
    mov ax, xPos            ; check if we are at the left-hand boundary
    cmp ax, 20d
    jg checkVertical
    mov al, 1               ; set xdir to 1 if at boundary
    mov xDir, al

checkVertical:
    mov ax, yPos            ; check if we are at the bottom boundary
    cmp ax, 160d
    jl notAtBottomBoundary
    xor al, al              ; set ydir to 0 if at boundary
    mov yDir, al
    jmp doHorizontalMovement

notAtBottomBoundary:
    mov ax, yPos            ; check if we are at the top boundary
    cmp ax, 20d
    jg doHorizontalMovement
    mov al, 1               ; set ydir to 1 if at boundary
    mov yDir, al

doHorizontalMovement:
    mov al, xDir            ; check which direction we're moving in
    test al, al
    jz moveLeft             ; if 0, skip to "move left"
    mov ax, xPos
    inc ax                  ; moving right, so increment ax
    mov xPos, ax            ; then store back into xPos
    jmp doVerticalMovement

moveLeft:
    mov ax, xPos            
    dec ax                  ; moving left, so decrement ax
    mov xPos, ax            ; then store back into xPos

doVerticalMovement:
    mov al, yDir            ; check which direction we're moving in
    test al, al
    jz moveUp               ; if 0, skip to "move up"
    mov ax, yPos
    inc ax                  ; moving down, so increment ax
    mov yPos, ax            ; then store back into yPos
    ret

moveUp:
    mov ax, yPos
    dec ax                  ; moving up, so decrement ax
    mov yPos, ax            ; then store back into yPos
    ret
moveBox endp

draw proc
    push cx                 ; we will probably mess with cx/bx,
    push bx                 ; push them to the stack

    mov ax, yPos            ; multiply the y position by 320
    mov bx, 320             ; to get the screen offset
    mul bx

    add ax, xPos               ; add the x coord for the final mem offset relative to 0
    add ax, offset screenArray ; add the address of screenArray for the final mem offset
    mov di, ax

    pop bx                  ; get the original bx (loop counter)
    push bx                 ; push it back as we're about to mess with it

    shr bx, 1               ; reduce speed of draw colour cycling
    shr bx, 1
    shr bx, 1
    mov al, bl              ; set the draw colour

    xor bx, bx

boxLoop:
    mov cx, 20              ; 20 pixels of box
    rep stosb               ; write to the screen
    add di, 300             ; move to the next vertical line (we already
                            ; incremented 20 pixels, so this is only 300)

    inc bx                  ; draw 20 rows of box
    cmp bx, 21
    jl boxLoop

    pop bx                  ; restore loop vars
    pop cx
    ret
draw endp

clearScreen proc
    mov cx, (320*200)/2         ; only need half the screen size as we clear
    cld                         ; one word at a time
    mov di, offset screenArray  ; load address of screen array
    mov ax, 0                   ; set colour to 0 (black)
    rep stosw                   ; write to video memory
    ret
clearScreen endp

copyScreen proc
    push es                     ; save the old es value

    mov cx, (320*200)/2         ; only need half the screen size as we copy
    cld                         ; one word at a time
    
    mov si, offset screenArray  ; set source pointer ds:si

    mov ax, 0A000h              ; set destination pointer es:di to video memory
    mov es, ax                  ; (A000:0000)
    mov di, 0
    
    rep movsw                   ; write to video memory

    pop es                      ; bring es back
    ret
copyScreen endp

bounce proc

    mov bx, seg dseg    ; load es with the base address of the offscreen buffer
    mov es, bx
    mov bx, 0

startLoop:
    mov canDrawNext, 0
    call clearScreen

    inc bx              ; colour cycling
    call moveBox
    call draw
    call copyScreen

waitForTimer:
    cmp canDrawNext, 1
    jnz waitForTimer

checkLoopCt:
    cmp bx, 20
    jnz notReadyYet
    mov al, 1        ; signal at least 20 loops have happened, this is a
    mov hasBegun, al ; cheat to avoid slow return release counting as a
                     ; keypress

notReadyYet:
    mov al, kbstate
    test al, al      ; check if we should exit due to keyboard state


    jz startLoop

retLoc:    
    ret
bounce endp

main proc
    push ds             ; push the PSP to the stack
    mov ax, seg dseg    ; populate ds with the correct segment
    mov ds, ax
    mov ax, 0F00h       ; get current video mode
    int 10h             ; call graphics interrupt
                        ; (AL = mode, AH = no columns, BH = active page)
    mov prevGfxMode, al ; store the previous mode
    mov ax, 0013h       ; AH = 0h (set video mode), AL = 13h (VGA 256 colour)
    int 10h             ; call graphics interrupt

    call setupInterrupts
    call bounce
    call restoreInterrupts

    xor ax, ax          ; zero ax
    mov al, prevGfxMode ; restore previous gfx mode to ah
    int 10h             ; call graphics interrupt

    pop ds
    mov ax, 4C00h       ; service 4c (exit program)
    int 21h             ; interrupt 21
main endp

end main

cseg ends
