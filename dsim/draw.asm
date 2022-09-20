SCREEN_WIDTH equ 320
SCREEN_HEIGHT equ 200
SCREEN_SIZE equ SCREEN_WIDTH*SCREEN_HEIGHT
SCREEN_ADDRESS equ 0xA0000

Line struc
        xStart  dd ?    ; X coordinate of leftmost pixel
        xEnd    dd ?    ; X coordinate of rightmost pixel
Line ends

LineList struc
        lineLength      dd ?    ; # of horizontal lines
        yStart          dd ?    ; Y coordinate of topmost line
        linePtr         dd ?    ; pointer to list of horizontal lines
LineList ends

Params struc
                    dd 2 dup(?) ; return address and pushed base pointer
        backBuffer  dd ?        ; pointer to back buffer
        lineListPtr dd ?        ; pointer to line list
        colour      dd ?        ; colour to fill
Params ends

CharPointer struc
                dd 2 dup(?)     ; return address and pushed base pointer
        cPtr    dd ?            ; the actual pointer passed to the function
CharPointer ends

.386
.model flat
.code
public _ASMDrawLineList, _ClearScreen, _CopyScreen
align 2

_ClearScreen proc
        push ebp                ; move stack pointer to base pointer
        mov ebp, esp            ; early on to give fixed reference
        push edi                
        
        mov edi, [ebp+cPtr]     ; set the dest address to the parameter

        mov eax, 0              ; this will always clear to black 
        mov ecx, SCREEN_SIZE/4  ; screen size / 4 as we're copying 4 bytes at a time

        rep stosd               ; set memory
        pop edi                 
        pop ebp                 ; clean up
        ret
_ClearScreen endp

_CopyScreen proc
        push ebp                ; move stack pointer to base pointer
        mov ebp, esp            ; early on to give fixed reference
        push esi
        push edi                ; we will clobber both esi and edi here

        mov esi, [ebp+cPtr]     ; set the src address to the pointer parameter
        mov edi, SCREEN_ADDRESS ; set the dest address to the VGA memory
        mov ecx, SCREEN_SIZE/4  ; screen size / 4 as we're copying 4 bytes at a time

        rep movsd               ; copy back buffer to screen

        pop edi
        pop esi
        pop ebp                 ; clean up
        ret
_CopyScreen endp

_ASMDrawLineList proc
        push ebp        ; Preserve stack frame
        mov ebp, esp    ; Point to our stack frame
        push esi        ; Preserve register variables
        push edi
        cld             ; make string instructions inc pointers

        mov esi, [ebp+lineListPtr] 
        mov ebx, [esi+linePtr]    ; point to the line descriptor of the first line
        mov ecx, [esi+yStart]
        mov esi, [esi+lineLength] ; # of scan lines to draw
        cmp esi, 0                ; check we have lines to draw
        jle FillDone              ; if 0, skip to done

        cmp ecx, 0                ; do we need to clip at top?
        jge MinYNotClipped

        neg ecx
        sub esi, ecx              ; draw this many fewer lines
        jle FillDone              ; jump ahead if no lines left
        shl ecx, 1
        shl ecx, 1
        shl ecx, 1                ; x8 for number of bytes per line
        add ebx, ecx
        mov ecx, 0                ; start at the top clip plan

MinYNotClipped:
        mov edx, esi
        add edx, ecx               ; find the bottom row
        cmp edx, SCREEN_HEIGHT     ; clipped at bottom?
        jle MaxYNotClipped
        sub edx, SCREEN_HEIGHT     ; how many rows clipped by
        sub esi, edx
        jle FillDone               ; skip ahead if all lines clipped
        
MaxYNotClipped:

        mov eax, SCREEN_WIDTH      ; need to move this around so eax gets multiplied
                                   ; before adding the screen address
        mul ecx
        add eax, [ebp+backBuffer]
        
        mov edx, eax              ; in theory edx now contains the start address
        
        mov eax, 0
        mov al, byte ptr [ebp+colour]   ; populate al with the colour
        mov ah, al                      ; duplicate to ah for REP STOSW

FillLoop:
        mov edi, 0
        mov ecx, 0
        mov edi, [ebx+xStart]    ; left edge of fill

        cmp edi, 0
        jge MinXNotClipped
        mov edi, 0               ; set left edge to 0 if clipped

MinXNotClipped:
        mov ecx, [ebx+xEnd]     ; right edge of fill
        cmp ecx, SCREEN_WIDTH   ; clipped on right edge?
        jl MaxXNotClipped
        mov ecx, SCREEN_WIDTH   ; Set to screen width - 1
        dec ecx

MaxXNotClipped:

        sub ecx, edi            ; set length
        js LineFillDone         ; skip if negative width
        inc ecx                 ; width of fill on this line
        add edi, edx            ; offset of left edge of fill
        test edi, 1             ; start with odd address?

        jz MainFill             ; skip if address is even

        stosb                   ; if odd, write the leading byte to align to word boundary
   
        dec ecx                 ; first byte done
        jz LineFillDone         ; skip if nothing left to draw

MainFill:
        shr ecx, 1              ; divide by 2 as we're filling a word at a time
        rep stosw               ; fill as many words as possible
        adc ecx, ecx            ; 1 if there's an odd trailing byte, else 0
        rep stosb               ; fill any odd trailing byte

LineFillDone:
        add ebx, size Line      ; point to next line descriptor
        add edx, SCREEN_WIDTH   ; offset destination by 1 line
        dec esi                 ; reduce count of lines left to fill
        jnz FillLoop            ; go back to fill loop if we still have lines left

FillDone:
        pop edi                 ; restore caller's registers
        pop esi
        pop ebp
        ret
_ASMDrawLineList endp
end