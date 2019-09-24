; X86Flags : C-1, H-10 Z-40, N-80, V-800

INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC
EXTERN	CalcEA_A: PROC

.data

.code

PUBLIC LBrn_R_A ; Op 1021 - Long Branch Never relative to PC
LBrn_R_A PROC
	add word ptr [pc_s], 2
	ret
LBrn_R_A ENDP

PUBLIC LBhi_R_A ; Op 1022 - Long Branch if Higher relative to PC
LBhi_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+CF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	jne bhiret
	call MemRead16_s
	add word ptr [pc_s], ax
bhiret:
	add rsp, 28h
	ret
LBhi_R_A ENDP

PUBLIC LBls_R_A ; Op 1023 - Long Branch if lower or same relative to PC
LBls_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+CF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	je blsret
	call MemRead16_s
	add word ptr [pc_s], ax
blsret:
	add rsp, 28h
	ret
LBls_R_A ENDP

PUBLIC LBhs_R_A ; Op 1024 - Long Branch if higher or same relative to PC
LBhs_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+CF_C_B], 1
	jne bhsret
	call MemRead16_s
	add word ptr [pc_s], ax
bhsret:
	add rsp, 28h
	ret
LBhs_R_A ENDP

PUBLIC LBcs_R_A ; Op 1025 - Long Branch if lower relative to PC
LBcs_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+CF_C_B], 1
	je bcsret
	call MemRead16_s
	add word ptr [pc_s], ax
bcsret:
	add rsp, 28h
	ret
LBcs_R_A ENDP

PUBLIC LBne_R_A ; Op 1026 - Long Branch if not equal relative to PC
LBne_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+ZF_C_B], 1
	jne bneret
	call MemRead16_s
	add word ptr [pc_s], ax
bneret:
	add rsp, 28h
	ret
LBne_R_A ENDP

PUBLIC LBeq_R_A ; Op 1027 - Long Branch if equal relative to PC
LBeq_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+ZF_C_B], 1
	je beqret
	call MemRead16_s
	add word ptr [pc_s], ax
beqret:
	add rsp, 28h
	ret
LBeq_R_A ENDP

PUBLIC LBvc_R_A ; Op 1028 - Branch if overflow clear relative to PC
LBvc_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+VF_C_B], 1
	jne bvcret
	call MemRead16_s
	add word ptr [pc_s], ax
bvcret:
	add rsp, 28h
	ret
LBvc_R_A ENDP

PUBLIC LBvs_R_A ; Op 1029 - Branch if overflow set relative to PC
LBvs_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+VF_C_B], 1
	je bvsret
	call MemRead16_s
	add word ptr [pc_s], ax
bvsret:
	add rsp, 28h
	ret
LBvs_R_A ENDP

PUBLIC LBpl_R_A ; Op 102A - Branch if plus relative to PC
LBpl_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+NF_C_B], 1
	jne bplret
	call MemRead16_s
	add word ptr [pc_s], ax
bplret:
	add rsp, 28h
	ret
LBpl_R_A ENDP

PUBLIC LBmi_R_A ; Op 102B - Branch if minus relative to PC
LBmi_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	test byte ptr [cc_s+NF_C_B], 1
	je bmiret
	call MemRead16_s
	add word ptr [pc_s], ax
bmiret:
	add rsp, 28h
	ret
LBmi_R_A ENDP

PUBLIC LBge_R_A ; Op 102C - Branch if greater and equal relative to PC
LBge_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	jne bgeret
	call MemRead16_s
	add word ptr [pc_s], ax
bgeret:
	add rsp, 28h
	ret
LBge_R_A ENDP

PUBLIC LBlt_R_A ; Op 102D - Branch if less than relative to PC
LBlt_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	je bltret
	call MemRead16_s
	add word ptr [pc_s], ax
bltret:
	add rsp, 28h
	ret
LBlt_R_A ENDP

PUBLIC LBgt_R_A ; Op 102E - Branch if greater relative to PC
LBgt_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	jne bgtret
	call MemRead16_s
	add word ptr [pc_s], ax
bgtret:
	add rsp, 28h
	ret
LBgt_R_A ENDP

PUBLIC LBle_R_A ; Op 102F - Branch if less or equal than relative to PC
LBle_R_A PROC
	sub rsp, 28h
	; Get Post Word at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	je bltret
	call MemRead16_s
	add word ptr [pc_s], ax
bltret:
	add rsp, 28h
	ret
LBle_R_A ENDP

PUBLIC Swi2_I_A ; Op 103F - Software wait for interupt 2
Swi2_I_A PROC
	sub rsp, 28h
	; EF = 1
	mov byte ptr [cc_s+EF_C_B], 1
	; push pc LSB
	movzx cx, byte ptr [pc_s+LSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push pc MSB
	movzx cx, byte ptr [pc_s+MSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push u LSB
	movzx cx, byte ptr [u_s+LSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push u MSB
	movzx cx, byte ptr [u_s+MSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push y LSB
	movzx cx, byte ptr [y_s+LSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push y MSB
	movzx cx, byte ptr [y_s+MSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push x LSB
	movzx cx, byte ptr [x_s+LSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push x MSB
	movzx cx, byte ptr [x_s+MSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push dp MSB
	movzx cx, byte ptr [dp_s+MSB]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; If MD native bit set push f & e
	test byte ptr [mdbits_s], MD_NATIVE
	je Swi21
	; push f
	movzx cx, byte ptr [q_s+FREG]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push e
	movzx cx, byte ptr [q_s+EREG]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
Swi21:
	; push b
	movzx cx, byte ptr [q_s+BREG]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push a
	movzx cx, byte ptr [q_s+AREG]
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; push cc
	call getcc_s
	movzx cx, al
	sub word ptr [s_s], 1
	mov dx, word ptr [s_s]
	call MemWrite8_s ; dx - address, cx - byte
	; PC = VSWI2
	mov cx, VSWI2
	call MemRead16_s
	mov word ptr [pc_s], ax
	add rsp, 28h
	ret
Swi2_I_A ENDP

PUBLIC Negd_I_A ; Op 1040 - Negate D reg
Negd_I_A PROC
	; Negate D Reg and save the FLAGS
	neg word ptr [q_s+DREG]
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Negd_I_A ENDP

PUBLIC Comd_I_A ; Op 1043 - Complement D reg
Comd_I_A PROC
	; Negate D Reg and save the FLAGS
	not word ptr [q_s+DREG]
	add word ptr [q_s+DREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+CF_C_B], 1
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	ret
Comd_I_A ENDP

PUBLIC Lsrd_I_A ; Op 1044 - Logical Shift Right D reg
Lsrd_I_A PROC
	; Shift Right D Reg and save the FLAGS
	shr word ptr [q_s+DREG], 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	ret
Lsrd_I_A ENDP

PUBLIC Rord_I_A ; Op 1046 - Rotate Right through Carry D reg
Rord_I_A PROC
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right D Reg and save the FLAGS
	rcr word ptr [q_s+DREG], 1
	; rcr only affects Carry.  Save the Carry first in dx then
	; add 0 to result to trigger Zero and Sign/Neg flags
	setc byte ptr [cc_s+CF_C_B]
	add word ptr [q_s+DREG], 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rord_I_A ENDP

PUBLIC Asrd_I_A ; Op 1047 - Arithmetic Shift Right D reg
Asrd_I_A PROC
	; Rotate Right D Reg and save the FLAGS
	sar word ptr [q_s+DREG], 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Asrd_I_A ENDP

PUBLIC Asld_I_A ; Op 1048 - Arithmetic Shift Left D reg
Asld_I_A PROC
	; Shift left D Reg and save the FLAGS
	sal word ptr [q_s+DREG], 1
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Asld_I_A ENDP

PUBLIC Rold_I_A ; Op 1049 - Rotate Left through carry D reg
Rold_I_A PROC
	mov ax, word ptr [q_s+DREG]
	; Save the top bit for the V flag calc
	mov dx, ax
	and dx, 08000h
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right D Reg and save the FLAGS
	rcl ax, 1
	mov word ptr [q_s+DREG], ax
	; rcl only affects Carry.  Save the Carry first in dx then
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dx, ax
	shr dx, 15
	mov byte ptr [cc_s+VF_C_B], dl
	; add 0 to result to trigger Zero and Sign/Neg flags
	add ax, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rold_I_A ENDP

PUBLIC Decd_I_A ; Op 104A - Decrement D Reg 
Decd_I_A PROC
	; Decrement the D Reg and save the FLAGS
	dec word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Decd_I_A ENDP

PUBLIC Incd_I_A ; Op 104C - Increment D Reg
Incd_I_A PROC
	Inc word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Incd_I_A ENDP

PUBLIC Tstd_I_A ; Op 104D - Test D Reg
Tstd_I_A PROC
	; Add D Reg with 0
	add word ptr [q_s+DREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Tstd_I_A ENDP

PUBLIC Clrd_I_A ; Op 104F - Clear D Reg
Clrd_I_A PROC
	mov word ptr [q_s+DREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clrd_I_A ENDP

PUBLIC Comw_I_A ; Op 1053 - Complement W reg
Comw_I_A PROC
	; Negate W Reg and save the FLAGS
	not word ptr [q_s+WREG]
	add word ptr [q_s+WREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+CF_C_B], 1
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	ret
Comw_I_A ENDP

PUBLIC Lsrw_I_A ; Op 1054 - Logical Shift Right W reg
Lsrw_I_A PROC
	; Shift Right W Reg and save the FLAGS
	shr word ptr [q_s+WREG], 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	ret
Lsrw_I_A ENDP

PUBLIC Rorw_I_A ; Op 1056 - Rotate Right through Carry W reg
Rorw_I_A PROC
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right W Reg and save the FLAGS
	rcr word ptr [q_s+WREG], 1
	; rcr only affects Carry.  Save the Carry first in dx then
	; add 0 to result to trigger Zero and Sign/Neg flags
	setc byte ptr [cc_s+CF_C_B]
	add word ptr [q_s+WREG], 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rorw_I_A ENDP

PUBLIC Rolw_I_A ; Op 1059 - Rotate Left through carry W reg
Rolw_I_A PROC
	mov ax, word ptr [q_s+WREG]
	; Save the top bit for the V flag calc
	mov dx, ax
	and dx, 08000h
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right W Reg and save the FLAGS
	rcl ax, 1
	mov word ptr [q_s+WREG], ax
	; rcl only affects Carry.  Save the Carry first in dx then
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dx, ax
	shr dx, 15
	mov byte ptr [cc_s+VF_C_B], dl
	; add 0 to result to trigger Zero and Sign/Neg flags
	add ax, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rolw_I_A ENDP

PUBLIC Decw_I_A ; Op 105A - Decrement W Reg 
Decw_I_A PROC
	; Decrement the A Reg and save the FLAGS
	dec word ptr [q_s+WREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Decw_I_A ENDP

PUBLIC Incw_I_A ; Op 105C - Increment W Reg
Incw_I_A PROC
	Inc word ptr [q_s+WREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Incw_I_A ENDP

PUBLIC Tstw_I_A ; Op 105D - Test W Reg
Tstw_I_A PROC
	; Add W Reg with 0
	add word ptr [q_s+WREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Tstw_I_A ENDP

PUBLIC Clrw_I_A ; Op 105F - Clear W Reg
Clrw_I_A PROC
	mov word ptr [q_s+WREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clrw_I_A ENDP

PUBLIC Subw_M_A ; Op 1080 - Subtract Immediate Memory from Reg W
Subw_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	sub word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subw_M_A ENDP

PUBLIC Cmpw_M_A ; Op 1081 - Compare Immediate Memory with Reg W
Cmpw_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpw_M_A ENDP

PUBLIC Sbcd_M_A ; Op 1082 - Subtract Immediate Memory from Reg D with borrow
Sbcd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcd_M_A ENDP

PUBLIC Cmpd_M_A ; Op 1083 - Compare Immediate Memory with Reg D
Cmpd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpd_M_A ENDP

PUBLIC Andd_M_A ; Op 1084 - And Immediate Memory with Reg D
Andd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	and word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Andd_M_A ENDP

PUBLIC Bitd_M_A ; Op 1085 - Bit test Immediate Memory with Reg D
Bitd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	and ax, word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bitd_M_A ENDP

PUBLIC Ldw_M_A ; Op 1086 - Load W from Memory
Ldw_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [q_s+WREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldw_M_A ENDP

PUBLIC Eord_M_A ; Op 1088 - Eor Immediate Memory with Reg D
Eord_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	xor word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eord_M_A ENDP

PUBLIC Adcd_M_A ; Op 1089 - Add Immediate Memory from Reg D with borrow
Adcd_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, word ptr [q_s+DREG]
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
  xor cx, word ptr [q_s+DREG]
	xor cx, ax
	and cx, 0100h
	mov byte ptr [cc_s+HF_C_B], ch
	add rsp, 28h
	ret
Adcd_M_A ENDP

PUBLIC Ord_M_A ; Op 108A - Or Immediate Memory with Reg D
Ord_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	or word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ord_M_A ENDP

PUBLIC Addw_M_A ; Op 108B - Add Immediate Memory from Reg W
Addw_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	add word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addw_M_A ENDP

PUBLIC Cmpy_M_A ; Op 108C - Compare Immediate Memory with Reg Y
Cmpy_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [y_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpy_M_A ENDP

PUBLIC Ldy_M_A ; Op 108E - Load Y from Memory
Ldy_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [y_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldy_M_A ENDP

PUBLIC Subw_D_A ; Op 1090 - Subtract DP Memory from Reg W
Subw_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	sub word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subw_D_A ENDP

PUBLIC Cmpw_D_A ; Op 1091 - Compare DP Memory with Reg W
Cmpw_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpw_D_A ENDP

PUBLIC Sbcd_D_A ; Op 1092 - Subtract DP Memory from Reg D with borrow
Sbcd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcd_D_A ENDP

PUBLIC Cmpd_D_A ; Op 1093 - Compare DP Memory with Reg D
Cmpd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpd_D_A ENDP

PUBLIC Andd_D_A ; Op 1094 - And DP Memory with Reg D
Andd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	and word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Andd_D_A ENDP

PUBLIC Bitd_D_A ; Op 1095 - Bit test DP Memory with Reg D
Bitd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	and ax, word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bitd_D_A ENDP

PUBLIC Ldw_D_A ; Op 1096 - Load W from DP Memory
Ldw_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [q_s+WREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldw_D_A ENDP

PUBLIC Stw_D_A ; Op 1097 Store W to DP Memory
Stw_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [q_s+WREG]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stw_D_A ENDP

PUBLIC Eord_D_A ; Op 1098 - Eor DP Memory with Reg D
Eord_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	xor word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eord_D_A ENDP

PUBLIC Adcd_D_A ; Op 1099 - Add DP Memory from Reg D with borrow
Adcd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov cx, word ptr [q_s+DREG]
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
  xor cx, word ptr [q_s+DREG]
	xor cx, ax
	and cx, 0100h
	mov byte ptr [cc_s+HF_C_B], ch
	add rsp, 28h
	ret
Adcd_D_A ENDP

PUBLIC Ord_D_A ; Op 109A - Or DP Memory with Reg D
Ord_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	or word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ord_D_A ENDP

PUBLIC Addw_D_A ; Op 109B - Add DP Memory from Reg W
Addw_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	add word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addw_D_A ENDP

PUBLIC Cmpy_D_A ; Op 109C - Compare DP Memory with Reg Y
Cmpy_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [y_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpy_D_A ENDP

PUBLIC Ldy_D_A ; Op 109E - Load Y from DP Memory
Ldy_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [y_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldy_D_A ENDP

PUBLIC Sty_D_A ; Op 109F Store Y to DP Memory
Sty_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [y_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Sty_D_A ENDP

PUBLIC Subw_X_A ; Op 10A0 - Subtract Extended Memory from Reg W
Subw_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	sub word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subw_X_A ENDP

PUBLIC Cmpw_X_A ; Op 10A1 - Compare Extended Memory with Reg W
Cmpw_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpw_X_A ENDP

PUBLIC Sbcd_X_A ; Op 10A2 - Subtract Extended Memory from Reg D with borrow
Sbcd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcd_X_A ENDP

PUBLIC Cmpd_X_A ; Op 10A3 - Compare Extended Memory with Reg D
Cmpd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpd_X_A ENDP

PUBLIC Andd_X_A ; Op 10A4 - And Extended Memory with Reg D
Andd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	and word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Andd_X_A ENDP

PUBLIC Bitd_X_A ; Op 10A5 - Bit test Extended Memory with Reg D
Bitd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	and ax, word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bitd_X_A ENDP

PUBLIC Ldw_X_A ; Op 10A6 - Load W from Extended Memory
Ldw_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [q_s+WREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldw_X_A ENDP

PUBLIC Stw_X_A ; Op 10A7 Store W to Extended Memory
Stw_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [q_s+WREG]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Stw_X_A ENDP

PUBLIC Eord_X_A ; Op 10A8 - Eor Extended Memory with Reg D
Eord_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	xor word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eord_X_A ENDP

PUBLIC Adcd_X_A ; Op 10A9 - Add Extended Memory from Reg D with borrow
Adcd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov cx, word ptr [q_s+DREG]
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
  xor cx, word ptr [q_s+DREG]
	xor cx, ax
	and cx, 0100h
	mov byte ptr [cc_s+HF_C_B], ch
	add rsp, 28h
	ret
Adcd_X_A ENDP

PUBLIC Ord_X_A ; Op 10AA - Or Extended Memory with Reg D
Ord_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	or word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ord_X_A ENDP

PUBLIC Addw_X_A ; Op 10AB - Add Extended Memory from Reg W
Addw_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	add word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addw_X_A ENDP

PUBLIC Cmpy_X_A ; Op 10AC - Compare Extended Memory with Reg Y
Cmpy_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	cmp word ptr [y_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpy_X_A ENDP

PUBLIC Ldy_X_A ; Op 10AE - Load Y from Extended Memory
Ldy_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [y_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldy_X_A ENDP

PUBLIC Sty_X_A ; Op 10AF Store Y to Extended Memory
Sty_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [y_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Sty_X_A ENDP

PUBLIC Subw_E_A ; Op 10B0 - Subtract Extended Memory from Reg W
Subw_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	sub word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subw_E_A ENDP

PUBLIC Cmpw_E_A ; Op 10B1 - Compare Extended Memory with Reg W
Cmpw_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpw_E_A ENDP

PUBLIC Sbcd_E_A ; Op 10B2 - Subtract with carry Extended Memory from Reg D
Sbcd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	; Substract with carry
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	sbb word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sbcd_E_A ENDP

PUBLIC Cmpd_E_A ; Op 10B3 - Compare Extended Memory with Reg D
Cmpd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpd_E_A ENDP

PUBLIC Andd_E_A ; Op 10B4 - And Extended Memory with Reg D
Andd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	and word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Andd_E_A ENDP

PUBLIC Bitd_E_A ; Op 10B5 - Bit Extended Memory with Reg D
Bitd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	and ax, word ptr [q_s+DREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Bitd_E_A ENDP

PUBLIC Ldw_E_A ; Op 10B6 - Load W from Extended Memory
Ldw_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [q_s+WREG], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldw_E_A ENDP

PUBLIC Stw_E_A ; Op 10B7 Store W to Memory Extended
Stw_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [q_s+WREG]
	call MemWrite16_s
	add word ptr [q_s+WREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Stw_E_A ENDP

PUBLIC Eord_E_A ; Op 10B8 - Xor Extended Memory with Reg D
Eord_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	xor word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Eord_E_A ENDP

PUBLIC Adcd_E_A ; Op 10B9 - Add Extended Memory from Reg D with borrow
Adcd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov cx, word ptr [q_s+DREG]
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	adc word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; the HF adjust flag is not so easily accessed
  xor cx, word ptr [q_s+DREG]
	xor cx, ax
	and cx, 0100h
	mov byte ptr [cc_s+HF_C_B], ch
	add rsp, 28h
	ret
Adcd_E_A ENDP

PUBLIC Ord_E_A ; Op 10BA - Or Extended Memory with Reg D
Ord_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	or word ptr [q_s+DREG], ax
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ord_E_A ENDP

PUBLIC Addw_E_A ; Op 10BB - Add Extended Memory with Reg W
Addw_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	add word ptr [q_s+WREG], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Addw_E_A ENDP

PUBLIC Cmpy_E_A ; Op 10BC - Compare Extended Memory with Reg Y
Cmpy_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [y_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpy_E_A ENDP

PUBLIC Ldy_E_A ; Op 10BE - Load Y from Extended Memory
Ldy_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [y_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldy_E_A ENDP

PUBLIC Sty_E_A ; Op 10BF Store Y to Extended Memory
Sty_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [y_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Sty_E_A ENDP

PUBLIC Lds_M_A ; Op 10CE - Load S from Memory
Lds_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov word ptr [s_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lds_M_A ENDP

PUBLIC Ldq_D_A ; Op 10DC - Load Q from DP Memory
Ldq_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
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
Ldq_D_A ENDP

PUBLIC Stq_D_A ; Op 10DD Store Q to DP Memory
Stq_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov ecx, dword ptr [q_s]
	add ecx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite32_s
	add rsp, 28h
	ret
Stq_D_A ENDP

PUBLIC Lds_D_A ; Op 10DE - Load S from DP Memory
Lds_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [s_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lds_D_A ENDP

PUBLIC Sts_D_A ; Op 10DF Store S to DP Memory
Sts_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	mov dl, al
	mov cx, word ptr [s_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Sts_D_A ENDP

PUBLIC Ldq_X_A ; Op 10EC - Load Q from Extended Memory
Ldq_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
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
Ldq_X_A ENDP

PUBLIC Stq_X_A ; Op 10ED Store Q to Extended Memory
Stq_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov ecx, dword ptr [q_s]
	add ecx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite32_s
	add rsp, 28h
	ret
Stq_X_A ENDP

PUBLIC Lds_X_A ; Op 10EE - Load S from Extended Memory
Lds_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov cx, ax
	; Save Address and use it to get a mem word
	call MemRead16_s
	mov word ptr [s_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lds_X_A ENDP

PUBLIC Sts_X_A ; Op 10EF Store S to Extended Memory
Sts_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov dx, ax
	mov cx, word ptr [s_s]
	add cx, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	call MemWrite16_s
	add rsp, 28h
	ret
Sts_X_A ENDP

PUBLIC Ldq_E_A ; Op 10FC - Load Q from Extended Memory
Ldq_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem 32bit
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
Ldq_E_A ENDP

PUBLIC Stq_E_A ; Op 10FD Store Q to Memory Extended
Stq_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov ecx, dword ptr [q_s]
	call MemWrite32_s
	add dword ptr [q_s], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Stq_E_A ENDP

PUBLIC Lds_E_A ; Op 10FE - Load S from Extended Memory
Lds_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	mov word ptr [s_s], ax
	; trigger and save flags
	add ax, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lds_E_A ENDP

PUBLIC Sts_E_A ; Op 10FF Store W to Memory Extended
Sts_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	mov cx, word ptr [s_s]
	call MemWrite16_s
	add word ptr [s_s], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Sts_E_A ENDP
END