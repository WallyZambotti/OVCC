; X86Flags : C-1, H-10 Z-40, N-80, V-800

INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC
EXTERN	CalcEA_A: PROC

.data

.code

PUBLIC Suba_M_A ; Op 80 - Subtract Immediate Memory from Reg A
Suba_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	sub byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Suba_M_A ENDP

PUBLIC Cmpa_M_A ; Op 81 - Compare Immediate Memory with Reg A
Cmpa_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	cmp byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpa_M_A ENDP

PUBLIC Sbca_M_A ; Op 82 - Subtract with carry Immediate Memory from Reg A
Sbca_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbca_M_A ENDP

PUBLIC Subd_M_A ; Op 83 - Subtract Immediate Memory from Reg D
Subd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	sub word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subd_M_A ENDP

PUBLIC Anda_M_A ; Op 84 - And Immediate Memory with Reg A
Anda_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	and byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Anda_M_A ENDP

PUBLIC Bita_M_A ; Op 85 - Bit Immediate Memory with Reg A
Bita_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	and al, byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bita_M_A ENDP

PUBLIC Lda_M_A ; Op 86 - Load A from Memory
Lda_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lda_M_A ENDP

PUBLIC Eora_M_A ; Op 88 - Xor Immediate Memory with Reg A
Eora_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	xor byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eora_M_A ENDP

PUBLIC Adca_M_A ; Op 89 - Add with carry Immediate Memory with Reg A
Adca_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adca_M_A ENDP

PUBLIC Ora_M_A ; Op 8A - Or Immediate Memory with Reg A
Ora_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	or byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ora_M_A ENDP

PUBLIC Adda_M_A ; Op 8B - Add Immediate Memory with Reg A
Adda_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	add byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adda_M_A ENDP

PUBLIC Cmpx_M_A ; Op 8C - Compare Immediate Memory with Reg X
Cmpx_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [x_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpx_M_A ENDP

PUBLIC Bsr_R_A ; Op 8D - Branch to Subroutine
Bsr_R_A PROC
	push rbx
	sub rsp, 20h
	; read post byte
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc
	inc bx
	mov word ptr [pc_s], bx
	call MemRead8_s
	; add post byte to pc
	movsx ax, al
	add word ptr [pc_s], ax
	; Save PC onto stack on byte at a time
	movzx cx, bl
	mov dx, word ptr [s_s]
	dec dx
	call MemWrite8_s ; dx - address, cx - byte
	movzx cx, bh
	mov dx, word ptr [s_s]
	sub dx, 2
	mov word ptr [s_s], dx ; update s_s
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 20h
	pop rbx
	ret
Bsr_R_A ENDP

PUBLIC Ldx_M_A ; Op 8E - Load X from Memory
Ldx_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [x_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldx_M_A ENDP

PUBLIC Suba_D_A ; Op 90 - Subtract DP Memory from Reg A
Suba_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Suba_D_A ENDP

PUBLIC Cmpa_D_A ; Op 91 - Compare DP Memory with Reg A
Cmpa_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpa_D_A ENDP

PUBLIC Sbca_D_A ; Op 92 - Subtract with carry DP Memory from Reg A
Sbca_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbca_D_A ENDP

PUBLIC Subd_D_A ; Op 93 - Subtract DP Memory from Reg D
Subd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	sub word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subd_D_A ENDP

PUBLIC Anda_D_A ; Op 94 - And DP Memory with Reg A
Anda_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Anda_D_A ENDP

PUBLIC Bita_D_A ; Op 95 - Bit DP Memory with Reg A
Bita_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bita_D_A ENDP

PUBLIC Lda_D_A ; Op 96 - Load A from DP Memory
Lda_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lda_D_A ENDP

PUBLIC Sta_D_A ; Op 97 Store A to DP Memory
Sta_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cl, byte ptr [q_s+AREG]
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite8_s
	add rsp, 28h
	ret
Sta_D_A ENDP

PUBLIC Eora_D_A ; Op 98 - Xor DP Memory with Reg A
Eora_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eora_D_A ENDP

PUBLIC Adca_D_A ; Op 99 - Add with carry DP Memory with Reg A
Adca_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adca_D_A ENDP

PUBLIC Ora_D_A ; Op 9A - Or DP Memory with Reg A
Ora_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ora_D_A ENDP

PUBLIC Adda_D_A ; Op 9B - Add DP Memory with Reg A
Adda_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adda_D_A ENDP

PUBLIC Cmpx_D_A ; Op 9C - Compare DP Memory with Reg X
Cmpx_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [x_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpx_D_A ENDP

PUBLIC Jsr_D_A ; Op 9D - Jump to Subroutine
Jsr_D_A PROC
	push rbx
	sub rsp, 20h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc
	inc bx
	call MemRead8_s
	; pc = dp | immediate byte
	mov ah, byte ptr [dp_s+MSB]
	mov word ptr [pc_s], ax
	; Save PC onto stack on byte at a time
	movzx cx, bl
	mov dx, word ptr [s_s]
	dec dx
	call MemWrite8_s ; dx - address, cx - byte
	movzx cx, bh
	mov dx, word ptr [s_s]
	sub dx, 2
	mov word ptr [s_s], dx ; update s_s
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 20h
	pop rbx
	ret
Jsr_D_A ENDP

PUBLIC Ldx_D_A ; Op 9E - Load X from DP Memory
Ldx_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [x_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldx_D_A ENDP

PUBLIC Stx_D_A ; Op 9F Store X to DP Memory
Stx_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [x_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stx_D_A ENDP

PUBLIC Suba_X_A ; Op A0 - Subtract Indexed Memory from Reg A
Suba_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Suba_X_A ENDP

PUBLIC Cmpa_X_A ; Op A1 - Compare Indexed Memory with Reg A
Cmpa_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpa_X_A ENDP

PUBLIC Sbca_X_A ; Op A2 - Subtract with carry Indexed Memory from Reg A
Sbca_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbca_X_A ENDP

PUBLIC Subd_X_A ; Op A3 - Subtract Indexed Memory from Reg D
Subd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	sub word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subd_X_A ENDP

PUBLIC Anda_X_A ; Op A4 - And Indexed Memory with Reg A
Anda_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Anda_X_A ENDP

PUBLIC Bita_X_A ; Op A5 - Bit Indexed Memory with Reg A
Bita_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bita_X_A ENDP

PUBLIC Lda_X_A ; Op A6 - Load A from Indexed Memory
Lda_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lda_X_A ENDP


PUBLIC Sta_X_A ; Op A7 Store A to Memory Indexed
Sta_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cl, byte ptr [q_s+AREG]
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite8_s
	add rsp, 28h
	ret
Sta_X_A ENDP

PUBLIC Eora_X_A ; Op A8 - Xor Indexed Memory with Reg A
Eora_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eora_X_A ENDP

PUBLIC Adca_X_A ; Op A9 - Add with carry Indexed Memory with Reg A
Adca_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adca_X_A ENDP

PUBLIC Ora_X_A ; Op AA - Or Indexed Memory with Reg A
Ora_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ora_X_A ENDP

PUBLIC Adda_X_A ; Op AB - Add Indexed Memory with Reg A
Adda_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adda_X_A ENDP

PUBLIC Cmpx_X_A ; Op AC - Compare Indexed Memory with Reg X
Cmpx_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [x_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpx_X_A ENDP

PUBLIC Jsr_X_A ; Op AD - Jump to Indexed Subroutine
Jsr_X_A PROC
	push rbx
	sub rsp, 20h
	call CalcEA_A
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc
	mov word ptr [pc_s], ax
	; Save PC onto stack on byte at a time
	movzx cx, bl
	mov dx, word ptr [s_s]
	dec dx
	call MemWrite8_s ; dx - address, cx - byte
	movzx cx, bh
	mov dx, word ptr [s_s]
	sub dx, 2
	mov word ptr [s_s], dx ; update s_s
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 20h
	pop rbx
	ret
Jsr_X_A ENDP

PUBLIC Ldx_X_A ; Op AE - Load X from Indexed Memory
Ldx_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [x_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldx_X_A ENDP

PUBLIC Stx_X_A ; Op AF Store X to Indexed Memory
Stx_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [x_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stx_X_A ENDP

PUBLIC Suba_E_A ; Op B0 - Subtract Extended Memory from Reg A
Suba_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Suba_E_A ENDP

PUBLIC Cmpa_E_A ; Op B1 - Compare Extended Memory with Reg A
Cmpa_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpa_E_A ENDP

PUBLIC Sbca_E_A ; Op B2 - Subtract with carry Extended Memory from Reg A
Sbca_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbca_E_A ENDP

PUBLIC Subd_E_A ; Op B3 - Subtract Extended Memory from Reg D
Subd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	sub word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subd_E_A ENDP

PUBLIC Anda_E_A ; Op B4 - And Indexed Memory with Reg A
Anda_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Anda_E_A ENDP

PUBLIC Bita_E_A ; Op B5 - Bit Indexed Memory with Reg A
Bita_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bita_E_A ENDP

PUBLIC Lda_E_A ; Op B6 - Load A from Indexed Memory
Lda_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lda_E_A ENDP


PUBLIC Sta_E_A ; Op B7 Store A to Memory Indexed
Sta_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	movzx cx, byte ptr [q_s+AREG]
	call MemWrite8_s
	add byte ptr [q_s+AREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Sta_E_A ENDP

PUBLIC Eora_E_A ; Op B8 - Xor Indexed Memory with Reg A
Eora_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eora_E_A ENDP

PUBLIC Adca_E_A ; Op B9 - Add with carry Indexed Memory with Reg A
Adca_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adca_E_A ENDP

PUBLIC Ora_E_A ; Op BA - Or Indexed Memory with Reg A
Ora_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ora_E_A ENDP

PUBLIC Adda_E_A ; Op BB - Add Indexed Memory with Reg A
Adda_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+AREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adda_E_A ENDP

PUBLIC Cmpx_E_A ; Op BC - Compare Indexed Memory with Reg X
Cmpx_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [x_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpx_E_A ENDP

PUBLIC Jsr_E_A ; Op BD - Jump to Indexed Subroutine
Jsr_E_A PROC
	push rbx
	sub rsp, 20h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc
	mov word ptr [pc_s], ax
	; Save PC onto stack on byte at a time
	movzx cx, bl
	mov dx, word ptr [s_s]
	dec dx
	call MemWrite8_s ; dx - address, cx - byte
	movzx cx, bh
	mov dx, word ptr [s_s]
	sub dx, 2
	mov word ptr [s_s], dx ; update s_s
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 20h
	pop rbx
	ret
Jsr_E_A ENDP

PUBLIC Ldx_E_A ; Op BE - Load X from Indexed Memory
Ldx_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [x_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldx_E_A ENDP

PUBLIC Stx_E_A ; Op BF Store X to Indexed Memory
Stx_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [x_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stx_E_A ENDP

PUBLIC Subb_M_A ; Op C0 - Subtract Immediate Memory from Reg B
Subb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	sub byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subb_M_A ENDP

PUBLIC Cmpb_M_A ; Op C1 - Compare Immediate Memory with Reg B
Cmpb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	cmp byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpb_M_A ENDP

PUBLIC Sbcb_M_A ; Op C2 - Subtract with carry Immediate Memory from Reg B
Sbcb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcb_M_A ENDP

PUBLIC Addd_M_A ; Op C3 - Add Immediate Memory to Reg D
Addd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	add word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addd_M_A ENDP

PUBLIC Andb_M_A ; Op C4 - And Immediate Memory with Reg B
Andb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	and byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Andb_M_A ENDP

PUBLIC Bitb_M_A ; Op C5 - Bit Immediate Memory with Reg B
Bitb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	and al, byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bitb_M_A ENDP

PUBLIC Ldb_M_A ; Op C6 - Load B from Memory
Ldb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldb_M_A ENDP

PUBLIC Eorb_M_A ; Op C8 - Xor Immediate Memory with Reg B
Eorb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	xor byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eorb_M_A ENDP

PUBLIC Adcb_M_A ; Op C9 - Add with carry Immediate Memory with Reg B
Adcb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adcb_M_A ENDP

PUBLIC Orb_M_A ; Op CA - Or Immediate Memory with Reg B
Orb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	or byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Orb_M_A ENDP

PUBLIC Addb_M_A ; Op CB - Add Immediate Memory with Reg B
Addb_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	add byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Addb_M_A ENDP

PUBLIC Ldd_M_A ; Op CC - Load D from Memory
Ldd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [q_s+DREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldd_M_A ENDP

PUBLIC Ldq_M_A ; Op CD - Load Q from Memory
Ldq_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 4
	call MemRead32_s
	mov dword ptr [q_s], eax
	; trigger and save flags
	add eax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldq_M_A ENDP

PUBLIC Ldu_M_A ; Op CE - Load U from Memory
Ldu_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [u_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldu_M_A ENDP

PUBLIC Subb_D_A ; Op D0 - Subtract DP Memory from Reg B
Subb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subb_D_A ENDP

PUBLIC Cmpb_D_A ; Op D1 - Compare DP Memory with Reg B
Cmpb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpb_D_A ENDP

PUBLIC Sbcb_D_A ; Op D2 - Subtract with carry DP Memory from Reg B
Sbcb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcb_D_A ENDP

PUBLIC Addd_D_A ; Op D3 - Add DP Memory to Reg D
Addd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	add word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addd_D_A ENDP

PUBLIC Andb_D_A ; Op D4 - And DP Memory with Reg B
Andb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Andb_D_A ENDP

PUBLIC Bitb_D_A ; Op D5 - Bit DP Memory with Reg B
Bitb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bitb_D_A ENDP

PUBLIC Ldb_D_A ; Op D6 - Load B from DP Memory
Ldb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldb_D_A ENDP

PUBLIC Stb_D_A ; Op D7 Store B to DP Memory
Stb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cl, byte ptr [q_s+BREG]
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite8_s
	add rsp, 28h
	ret
Stb_D_A ENDP

PUBLIC Eorb_D_A ; Op D8 - Xor DP Memory with Reg B
Eorb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eorb_D_A ENDP

PUBLIC Adcb_D_A ; Op D9 - Add with carry DP Memory with Reg B
Adcb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adcb_D_A ENDP

PUBLIC Orb_D_A ; Op DA - Or DP Memory with Reg B
Orb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Orb_D_A ENDP

PUBLIC Addb_D_A ; Op DB - Add DP Memory with Reg B
Addb_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Addb_D_A ENDP

PUBLIC Ldd_D_A ; Op DC - Load D from DP Memory
Ldd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [q_s+DREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldd_D_A ENDP

PUBLIC Std_D_A ; Op DD Store D to DP Memory
Std_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [q_s+DREG]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Std_D_A ENDP

PUBLIC Ldu_D_A ; Op DE - Load U from DP Memory
Ldu_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [u_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldu_D_A ENDP

PUBLIC Stu_D_A ; Op DF Store U to DP Memory
Stu_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [u_s]
	; Save flags NF+ZF+VF
	add cx ,0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Defer the write until after the flags are saved
	call MemWrite16_s
	add rsp, 28h
	ret
Stu_D_A ENDP

PUBLIC Subb_X_A ; Op E0 - Subtract Indexed Memory from Reg B
Subb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subb_X_A ENDP

PUBLIC Cmpb_X_A ; Op E1 - Compare Indexed Memory with Reg B
Cmpb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpb_X_A ENDP

PUBLIC Sbcb_X_A ; Op E2 - Subtract with carry Indexed Memory from Reg B
Sbcb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcb_X_A ENDP

PUBLIC Addd_X_A ; Op E3 - Add Indexed Memory to Reg D
Addd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	add word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addd_X_A ENDP

PUBLIC Andb_X_A ; Op E4 - And Indexed Memory with Reg B
Andb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Andb_X_A ENDP

PUBLIC Bitb_X_A ; Op E5 - Bit Indexed Memory with Reg B
Bitb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bitb_X_A ENDP

PUBLIC Ldb_X_A ; Op E6 - Load B from Indexed Memory
Ldb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldb_X_A ENDP


PUBLIC Stb_X_A ; Op E7 Store B to Memory Indexed
Stb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	movzx cx, byte ptr [q_s+BREG]
	call MemWrite8_s
	add byte ptr [q_s+BREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Stb_X_A ENDP

PUBLIC Eorb_X_A ; Op E8 - Xor Indexed Memory with Reg B
Eorb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eorb_X_A ENDP

PUBLIC Adcb_X_A ; Op E9 - Add with carry Indexed Memory with Reg B
Adcb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adcb_X_A ENDP

PUBLIC Orb_X_A ; Op EA - Or Indexed Memory with Reg B
Orb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Orb_X_A ENDP

PUBLIC Addb_X_A ; Op EB - Add Indexed Memory with Reg B
Addb_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Addb_X_A ENDP

PUBLIC Ldd_X_A ; Op EC - Load D from Indexed Memory
Ldd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [q_s+DREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldd_X_A ENDP

PUBLIC Std_X_A ; Op ED Store D to Indexed Memory
Std_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [q_s+DREG]
	call MemWrite16_s
	add word ptr [q_s+DREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Std_X_A ENDP

PUBLIC Ldu_X_A ; Op EE - Load U from Indexed Memory
Ldu_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [u_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldu_X_A ENDP

PUBLIC Stu_X_A ; Op EF Store U to Indexed Memory
Stu_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [u_s]
	call MemWrite16_s
	add word ptr [u_s], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Stu_X_A ENDP

PUBLIC Subb_E_A ; Op F0 - Subtract Extended Memory from Reg B
Subb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subb_E_A ENDP

PUBLIC Cmpb_E_A ; Op F1 - Compare Extended Memory with Reg B
Cmpb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpb_E_A ENDP

PUBLIC Sbcb_E_A ; Op F2 - Subtract with carry Extended Memory from Reg B
Sbcb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcb_E_A ENDP

PUBLIC Addd_E_A ; Op F3 - Add Extended Memory to Reg D
Addd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	add word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addd_E_A ENDP

PUBLIC Andb_E_A ; Op F4 - And Extended Memory with Reg B
Andb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Andb_E_A ENDP

PUBLIC Bitb_E_A ; Op F5 - Bit Extended Memory with Reg B
Bitb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	and al, byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Bitb_E_A ENDP

PUBLIC Ldb_E_A ; Op F6 - Load B from Extended Memory
Ldb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldb_E_A ENDP

PUBLIC Stb_E_A ; Op F7 Store B to Memory Extended
Stb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	movzx cx, byte ptr [q_s+BREG]
	call MemWrite8_s
	add byte ptr [q_s+BREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Stb_E_A ENDP

PUBLIC Eorb_E_A ; Op F8 - Xor Extended Memory with Reg B
Eorb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	xor byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eorb_E_A ENDP

PUBLIC Adcb_E_A ; Op F9 - Add with carry Extended Memory with Reg B
Adcb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	; Restore carry flag and add with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF+HF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Adcb_E_A ENDP

PUBLIC Orb_E_A ; Op FA - Or Extended Memory with Reg B
Orb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	or byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Orb_E_A ENDP

PUBLIC Addb_E_A ; Op FB - Add Extended Memory with Reg B
Addb_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+BREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
	lahf
	bt ax, HF_B+8 ; bit 12 is HF - adjust
	setc byte ptr [cc_s+HF_C_B]
	add rsp, 28h
	ret
Addb_E_A ENDP

PUBLIC Ldd_E_A ; Op FC - Load D from Extended Memory
Ldd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [q_s+DREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldd_E_A ENDP

PUBLIC Std_E_A ; Op FD Store D to Extended Memory
Std_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [q_s+DREG]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Std_E_A ENDP

PUBLIC Ldu_E_A ; Op FE - Load U from Extended Memory
Ldu_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [u_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldu_E_A ENDP

PUBLIC Stu_E_A ; Op FF Store U to Extended Memory
Stu_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [u_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stu_E_A ENDP
END