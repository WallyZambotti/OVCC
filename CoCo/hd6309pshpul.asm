INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC

.data

.code

PUBLIC Pshs_M_A ; Op 34 Push to Stack S
Pshs_M_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov bx, ax
	; push pc
	test bx, 80h
	je pshs_1
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [pc_s+LSB]
	call MemWrite8_s
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [pc_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshs_1:
	; push u
	test bx, 40h
	je pshs_2
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [u_s+LSB]
	call MemWrite8_s
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [u_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshs_2:
	; push y
	test bx, 20h
	je pshs_3
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [y_s+LSB]
	call MemWrite8_s
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [y_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshs_3:
	; push x
	test bx, 10h
	je pshs_4
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [x_s+LSB]
	call MemWrite8_s
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [x_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshs_4:
	; push dp
	test bx, 8h
	je pshs_5
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [dp_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 1
pshs_5:
	; push b
	test bx, 4h
	je pshs_6
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [q_s+BREG]
	call MemWrite8_s
	add [CycleCounter], 1
pshs_6:
	; push a
	test bx, 2h
	je pshs_7
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [q_s+AREG]
	call MemWrite8_s
	add [CycleCounter], 1
pshs_7:
	; push cc
	test bx, 1h
	je pshs_8
	dec word ptr [s_s]
	call getcc_s
	mov cx, ax
	mov dx, word ptr [s_s]
	call MemWrite8_s
	add [CycleCounter], 1
pshs_8:
	add rsp, 28h
	ret
Pshs_M_A ENDP

PUBLIC Puls_M_A ; Op 35 Pull from Stack S
Puls_M_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov bx, ax
	; pull cc
	test bx, 1h
	je puls_8
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov cx, ax
	call setcc_s
	add [CycleCounter], 1
puls_8:
	; pull a
	test bx, 2h
	je puls_7
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	add [CycleCounter], 1
puls_7:
	; pull b
	test bx, 4h
	je puls_6
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	add [CycleCounter], 1
puls_6:
	; pull dp
	test bx, 8h
	je puls_5
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [dp_s+MSB], al
	add [CycleCounter], 1
puls_5:
	; pull x
	test bx, 10h
	je puls_4
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [x_s+MSB], al
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [x_s+LSB], al
	add [CycleCounter], 2
puls_4:
	; pull y
	test bx, 20h
	je puls_3
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [y_s+MSB], al
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [y_s+LSB], al
	add [CycleCounter], 2
puls_3:
	; pull u
	test bx, 40h
	je puls_2
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [u_s+MSB], al
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [u_s+LSB], al
	add [CycleCounter], 2
puls_2:
	; pull pc
	test bx, 80h
	je puls_1
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+MSB], al
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+LSB], al
	add [CycleCounter], 2
puls_1:
	add rsp, 28h
	ret
Puls_M_A ENDP

PUBLIC Pshu_M_A ; Op 36 Push to Stack U
Pshu_M_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov bx, ax
	; push pc
	test bx, 80h
	je pshu_1
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [pc_s+LSB]
	call MemWrite8_s
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [pc_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshu_1:
	; push s
	test bx, 40h
	je pshu_2
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [s_s+LSB]
	call MemWrite8_s
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [s_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshu_2:
	; push y
	test bx, 20h
	je pshu_3
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [y_s+LSB]
	call MemWrite8_s
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [y_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshu_3:
	; push x
	test bx, 10h
	je pshu_4
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [x_s+LSB]
	call MemWrite8_s
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [x_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 2
pshu_4:
	; push dp
	test bx, 8h
	je pshu_5
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [dp_s+MSB]
	call MemWrite8_s
	add [CycleCounter], 1
pshu_5:
	; push b
	test bx, 4h
	je pshu_6
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [q_s+BREG]
	call MemWrite8_s
	add [CycleCounter], 1
pshu_6:
	; push a
	test bx, 2h
	je pshu_7
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [q_s+AREG]
	call MemWrite8_s
	add [CycleCounter], 1
pshu_7:
	; push cc
	test bx, 1h
	je pshu_8
	dec word ptr [u_s]
	call getcc_s
	mov cx, ax
	mov dx, word ptr [u_s]
	call MemWrite8_s
	add [CycleCounter], 1
pshu_8:
	add rsp, 28h
	ret
Pshu_M_A ENDP

PUBLIC Pulu_M_A ; Op 37 Pull from Stack U
Pulu_M_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov bx, ax
	; pull cc
	test bx, 1h
	je pulu_8
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov cx, ax
	call setcc_s
	add [CycleCounter], 1
pulu_8:
	; pull a
	test bx, 2h
	je pulu_7
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	add [CycleCounter], 1
pulu_7:
	; pull b
	test bx, 4h
	je pulu_6
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	add [CycleCounter], 1
pulu_6:
	; pull dp
	test bx, 8h
	je pulu_5
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [dp_s+MSB], al
	add [CycleCounter], 1
pulu_5:
	; pull x
	test bx, 10h
	je pulu_4
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [x_s+MSB], al
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [x_s+LSB], al
	add [CycleCounter], 2
pulu_4:
	; pull y
	test bx, 20h
	je pulu_3
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [y_s+MSB], al
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [y_s+LSB], al
	add [CycleCounter], 2
pulu_3:
	; pull s
	test bx, 40h
	je pulu_2
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [s_s+MSB], al
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [s_s+LSB], al
	add [CycleCounter], 2
pulu_2:
	; pull pc
	test bx, 80h
	je pulu_1
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [pc_s+MSB], al
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [pc_s+LSB], al
	add [CycleCounter], 2
pulu_1:
	add rsp, 28h
	ret
Pulu_M_A ENDP

PUBLIC Pshsw_A ; Op 1038 - Push W onto the S stack
Pshsw_A PROC
	sub rsp, 28h
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [q_s+FREG]
	call MemWrite8_s
	dec word ptr [s_s]
	mov dx, word ptr [s_s]
	movzx cx, byte ptr [q_s+EREG]
	call MemWrite8_s	
	add rsp, 28h
	ret
Pshsw_A ENDP

PUBLIC Pulsw_A ; Op 1039 - Push W onto the S stack
Pulsw_A PROC
	sub rsp, 28h
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	add rsp, 28h
	ret
Pulsw_A ENDP

PUBLIC Pshuw_A ; Op 103A - Push W onto the U stack
Pshuw_A PROC
	sub rsp, 28h
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [q_s+FREG]
	call MemWrite8_s
	dec word ptr [u_s]
	mov dx, word ptr [u_s]
	movzx cx, byte ptr [q_s+EREG]
	call MemWrite8_s	
	add rsp, 28h
	ret
Pshuw_A ENDP

PUBLIC Puluw_A ; Op 103B - Push W onto the U stack
Puluw_A PROC
	sub rsp, 28h
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	mov cx, word ptr [u_s]
	inc word ptr [u_s]
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	add rsp, 28h
	ret
Puluw_A ENDP
END
