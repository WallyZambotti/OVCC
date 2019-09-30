INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC

.data

.code

PUBLIC Exg_M_A ; Op 1E - Exchange Registers
Exg_M_A PROC
SOURCE EQU 2
DEST EQU 4
	push rbx
	sub rsp, 20h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rbx, ax
	; Source Reg = postbyte >> 4;
	shr al, 4
	; Dest = poastbyte & 0x0f
	and bl, 0FH
	mov word ptr [rsp+SOURCE], ax
	mov word ptr [rsp+DEST], bx
	; ccbits_s = getcc_s()
	call getcc_s
	mov byte ptr [ccbits_s], al
	; if (source & 8) == (dest & 8)
	mov ax, word ptr [rsp+SOURCE]
	and al, 8
	mov bx, word ptr [rsp+DEST]
	and bl, 8
	cmp al, bl
	jne exgdiffsize
	; if dest & 8
	cmp bl, 8
	jne else16
	movzx rax, word ptr [rsp+SOURCE]
	and al, 7
	movzx r8w, al
	movzx rbx, word ptr [rsp+DEST]
	and bl, 7
	movzx r9w, bl
	lea rcx, word ptr [ureg8_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, word ptr [ureg8_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov al, byte ptr [rcx]
	mov bl, byte ptr [rdx]
	cmp r8w, 4 ; if source is 4 or 5 do nothing
	je exgdest
	cmp r8w, 5
	je exgdest
	mov byte ptr [rcx], bl
exgdest:
	cmp r9w, 4 ; if dest is 4 or 5 do nothing
	je exgret
	cmp r9w, 5
	je exgret
	mov byte ptr [rdx], al
	jmp exgret
else16:
	movzx rax, word ptr [rsp+SOURCE]
	and al, 7
	movzx rbx, word ptr [rsp+DEST]
	and bl, 7
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov ax, word ptr [rcx]
	mov bx, word ptr [rdx]
	mov word ptr [rcx], bx
	mov word ptr [rdx], ax
	jmp exgret
exgdiffsize:
	; if dest & 8
	cmp bl, 8
	jne noswap
	mov bx, word ptr [rsp+SOURCE]
	mov ax, word ptr [rsp+DEST]
	jmp exgcont
noswap:
	mov ax, word ptr [rsp+SOURCE]
	mov bx, word ptr [rsp+DEST]
exgcont:
	and al, 7 ; source & 7
	movzx rcx, ax
	and bl, 7
; switch (source) {
	lea rdx, exgswitch
	mov rdx, [rdx+rcx*8]
	jmp rdx
exgswitch:
	QWORD exgcase00
	QWORD exgcase01
	QWORD exgcase02
	QWORD exgcase03
	QWORD exgcase04
	QWORD exgcase05
	QWORD exgcase06
	QWORD exgcase07
exgcase04: ; case 4:
exgcase05: ; case 5:
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rbx*8]
	mov word ptr [rcx], 0
	jmp exgret
exgcase00: ; case 0:
exgcase03: ; case 3:
exgcase06: ; case 6:
	lea rcx, byte ptr [ureg8_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov al, byte ptr [rcx]
	mov ah, al
	mov bx, word ptr [rdx]
	mov byte ptr [rcx], bh
	mov word ptr [rdx], ax
	jmp exgret
exgcase01: ; case 1:
exgcase02: ; case 2:
exgcase07: ; case 7:
	lea rcx, byte ptr [ureg8_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov al, byte ptr [rcx]
	mov ah, al
	mov bx, word ptr [rdx]
	mov byte ptr [rcx], bl
	mov word ptr [rdx], ax
; } switch end
exgret: 
	movzx cx, byte ptr [ccbits_s]
	call setcc_s
	add rsp, 20h
	pop rbx	
	ret
Exg_M_A ENDP

PUBLIC Tfr_M_A ; Op 1F - Transfer Registers
Tfr_M_A PROC
SOURCE EQU 2
DEST EQU 4
	push rbx
	sub rsp, 20h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rbx, ax
	; Source Reg = postbyte >> 4;
	shr al, 4
	; Dest = poastbyte & 0x0f
	and bl, 0FH
	mov word ptr [rsp+SOURCE], ax
	mov word ptr [rsp+DEST], bx
	; ccbits_s = getcc_s()
	call getcc_s
	mov byte ptr [ccbits_s], al
	; if (dest < 8)
	mov ax, word ptr [rsp+SOURCE]
	mov bx, word ptr [rsp+DEST]
	cmp bx, 8
	jge tfrelse1
	; if (source < 8)
	cmp ax, 8
	jge tfrelse2
	; *xfreg16[dest] = *xfreg16[source]
	lea rcx, byte ptr [xfreg16_s]
	mov rdx, rcx
	mov rcx, qword ptr [rcx+rax*8]
	mov rdx, qword ptr [rdx+rbx*8]
	mov ax, word ptr [rcx]
	mov word ptr [rdx], ax
	jmp tfrret
tfrelse2:
	; source &= 7
	and rax, 7
	; *xfreg16[dest] = (*ureg8[source]<<8) | *ureg8[source]
	lea rcx, byte ptr [ureg8_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, byte ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov al, byte ptr [rcx]
	mov ah, al
	mov word ptr [rdx], ax
	jmp tfrret
tfrelse1:
	; dest &= 7
	and bl, 7
	; if (source < 8)
	cmp al, 8
	jge tfrelse3
	; switch (dest&7)
	movzx rcx, bl
	lea rdx, tfrswitch
	mov rdx, [rdx+rcx*8]
	jmp rdx
tfrswitch:
	QWORD tfrcase00
	QWORD tfrcase01
	QWORD tfrcase02
	QWORD tfrcase03
	QWORD tfrswchend
	QWORD tfrswchend
	QWORD tfrcase06
	QWORD tfrcase07
tfrcase00: ; case 0:
tfrcase03: ; case 3:
tfrcase06: ; case 6:
	; *ureg8[dest] = *xfreg16[source] >> 8
	lea rcx, byte ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, byte ptr [ureg8_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov ax, word ptr [rcx]
	mov byte ptr [rdx], ah
	jmp tfrccbits
tfrcase01: ; case 1:
tfrcase02: ; case 2:
tfrcase07: ; case 7:
	; *ureg8[dest] = *xfreg16[source] & 0xff
	lea rcx, byte ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	lea rdx, byte ptr [ureg8_s]
	mov rdx, qword ptr [rdx+rbx*8]
	mov ax, word ptr [rcx]
	mov byte ptr [rdx], al
	jmp tfrccbits
tfrelse3:
	; *ureg8[dest] = *ureg8[source]
	and rax, 7
	lea rcx, byte ptr [ureg8_s]
	mov rdx, rcx
	mov rcx, qword ptr [rcx+rax*8]
	mov rdx, qword ptr [rdx+rbx*8]
	mov al, byte ptr [rcx]
	mov byte ptr [rdx], al
tfrccbits:
	movzx cx, byte ptr [ccbits_s]
	call setcc_s
tfrswchend:
tfrret: 
	add rsp, 20h
	pop rbx	
	ret
Tfr_M_A ENDP
END