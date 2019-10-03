; X86Flags : C-1, H-10 Z-40, N-80, V-800

INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC
EXTERN	CalcEA_A: PROC

.data

.code

PUBLIC Neg_D_A ; Op 00 - Negate DP memory byte
Neg_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Negate the byte and save the FLAGS
	neg al
	; Save flags CF+VF+ZF+NF
	setc byte ptr [cc_s+CF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write negated byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Neg_D_A ENDP

PUBLIC Oim_D_A ; Op 01 - Logical OR DP memory byte with immediate value
Oim_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Or Mem Byte with saved Immediate Value
	or al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Write Or'd byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Oim_D_A ENDP

PUBLIC Aim_D_A ; Op 02 - Logical AND DP memory byte with immediate value
Aim_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Write Or'd byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Aim_D_A ENDP

PUBLIC Com_D_A ; Op 03 - Complement DP memory byte
Com_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Negate the byte and save the FLAGS
	not al
	add al, 0 ; x86 not doesn't affect any flags so trigger
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 1 ; Always Set C flag
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Com_D_A ENDP

PUBLIC Lsr_D_A ; Op 04 - Logical Shift Right DP memory byte
Lsr_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Shift Right the byte and save the FLAGS
	shr al, 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	; Write shifted byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Lsr_D_A ENDP

PUBLIC Eim_D_A ; Op 05 - Logical XOR DP memory byte with immediate value
Eim_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Xor Mem Byte with saved Immediate Value
	xor al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Write Xor'd byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s  ; dx - address, cx - byte
	add rsp, 28h
	ret
Eim_D_A ENDP

PUBLIC Ror_D_A ; Op 06 - Rotate Right through Carry DP memory byte
Ror_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right the byte and save the FLAGS
	rcr al, 1
	; rcr only affects Carry.  Save the Carry flag first
	setc byte ptr [cc_s+CF_C_B]
	; add 0 to result to trigger Zero and Sign/Neg flags
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Ror_D_A ENDP

PUBLIC Asr_D_A ; Op 07 - Arithmetic Shift Right DP memory byte
Asr_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Rotate Right the byte and save the FLAGS
	sar al, 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asr_D_A ENDP

PUBLIC Asl_D_A ; Op 08 - Arithmetic Shift Left DP memory byte
Asl_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Shift left the byte and save the FLAGS
	sal al, 1
	; Save flags NF+ZF+CF+VF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asl_D_A ENDP

PUBLIC Rol_D_A ; Op 09 - Rotate Left through Carry DP memory byte
Rol_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Save the top bit for the V flag calc
	mov dl, al
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	; Rotate left the byte and save the FLAGS
	rcl al, 1
	; rcl only affects Carry.  Save the Carry first
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dl, al
	shr dl, 7
	mov byte ptr [cc_s+VF_C_B], dl
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte (ah) to address (saved on stack)
	movzx cx, al
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Rol_D_A ENDP

PUBLIC Dec_D_A ; Op 0A - Decrement DP memory byte
Dec_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Decrement the A Reg and save the FLAGS
	dec al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Dec_D_A ENDP

PUBLIC Tim_D_A ; Op 0B - Test DP memory byte with immediate value
Tim_D_A PROC
PBYTE EQU 2
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Use Address to get a mem byte
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Tim_D_A ENDP

PUBLIC Inc_D_A ; Op 0C - Increment DP memory byte
Inc_D_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], cx
	call MemRead8_s
	; Increment the byte and save the FLAGS
	Inc al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Inc_D_A ENDP

PUBLIC Tst_D_A ; Op 0D - Test DP memory byte
Tst_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	movzx rcx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [dp_s]
	or cx, ax
	; Save Address and use it to get a mem byte
	call MemRead8_s
	; Add Mem Byte with 0
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Tst_D_A ENDP

PUBLIC Jmp_D_A ; Op 0E - Jump to DP memory address
Jmp_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	movzx rcx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [dp_s]
	or cx, ax
	; Update PC_S with Address
	mov word ptr [pc_s], cx
	add rsp, 28h
	ret
Jmp_D_A ENDP

PUBLIC Clr_D_A ; Op 0F - Clear DP memory byte
Clr_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	movzx rcx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rdx, word ptr [dp_s]
	or dx, ax
	; Use Address to clear byte
	mov cx, 0
	call MemWrite8_s ; dx - address, cx - byte
	; Save flags NF+ZF+VF+CF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	add rsp, 28h
	ret
Clr_D_A ENDP

PUBLIC Nop_I_A ; Op 12 - No Operation
Nop_I_A PROC
	ret
Nop_I_A ENDP

PUBLIC Sync_I_A ; Op 13 - Synchronisze with Interupt
Sync_I_A PROC
	mov eax, dword ptr [gCycleFor]
	mov dword ptr [CycleCounter], eax
	mov byte ptr [SyncWaiting_s], 1
	ret
Sync_I_A ENDP

PUBLIC Sexw_I_A ; Op 14 - Sign Extend W into Q
Sexw_I_A PROC
	sub rsp, 28h
	movsx ecx, word ptr [q_s] ; W Reg Sign extend 16 bits into 32
	mov dword ptr [q_s], ecx
	; trigger and save flags
	add ecx, 0 ;
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	add rsp, 28h
	ret
Sexw_I_A ENDP

PUBLIC Lbra_R_A ; Op 16 - Long Branch Always
Lbra_R_A PROC
HBYTE EQU 2
	push rbx
	sub rsp, 20h
	; Calc destination branch address by Adding the two Bytes at [PC+1] (high byte) and [PC+2] (low byte) with PC+2 reg
	; Get first byte high byte
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc_s into temp
	inc bx ; inc temp pc_s
	call MemRead8_s
	mov word ptr [rsp+HBYTE], ax ; save high byte
	; Get second byte low byte @ pc_s
	mov cx, bx
	inc bx ; inc temp pc_s
	call MemRead8_s
	; combine low and high bytes to make 16 bit 2 complements offset
	mov ah, byte ptr [rsp+HBYTE] ; move the high byte to high position ; ax now contains 16 bit offset
	add bx, ax ; bx now contains pc_s(+2) + offset
	mov word ptr [pc_s], bx
	add rsp, 20h
	pop rbx
	ret
Lbra_R_A ENDP

PUBLIC Lbsr_R_A ; Op 17 - Long Branch to Subroutine
Lbsr_R_A PROC
HBYTE EQU 2
	push rbx
	sub rsp, 20h
	; Calc destination branch address by Adding the two Bytes at [PC+1] (high byte) and [PC+2] (low byte) with PC+2 reg
	; Get first byte high byte
	mov cx, word ptr [pc_s]
	mov bx, cx ; save pc_s into temp
	inc bx ; inc temp pc_s
	call MemRead8_s
	mov byte ptr [rsp+HBYTE], al ; save high byte
	; Get second byte low byte @ pc_s
	mov cx, bx
	inc bx ; inc temp pc_s
	call MemRead8_s
	; combine low and high bytes to make 16 bit 2 complements offset
	mov ah, byte ptr [rsp+HBYTE] ; move the high byte to high position ; ax now contains 16 bit offset
	add ax, bx ; ax now contains pc_s(+2) + offset
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
Lbsr_R_A ENDP

PUBLIC Daa_I_A ; Op 19 - Decimal Adjust A register
Daa_I_A PROC
	sub rsp, 28h
	mov al, byte ptr [q_s+AREG] ; A reg
	movzx dx, al ; temp A reg
	;
	mov ah, al ; msn
	and ah, 0f0h ; msn & F0h
	and al, 0fh ; lsn & Fh
	mov cx, 0 ; temp
	; if Half Carry Flag set or lsn > 9
	test byte ptr [cc_s+HF_C_B], 1 ; Half carry
	jnz ifthen1
	cmp al, 9
	jbe ifend1
ifthen1:
	or cx, 06h ; then temp |= 6;
ifend1:
	; if msn > 80h and lsn > 9
	cmp ah, 080h
	jbe ifend2
	cmp al, 9
	jbe ifend2
	or cx, 060h ; then temp |= 60h;
ifend2:
	; if msn > 90h or carry flag
	cmp ah, 090h
	ja ifthen3
	test byte ptr [cc_s+CF_C_B], 1 ; carry
	jz ifend3
ifthen3:
	or cx, 060h ; then temp |= 60h;
ifend3:
	add dx, cx ; temp A reg += temp
	mov byte ptr [q_s+AREG], dl ; save result back to A reg
	mov al, dl
	shr dx, 8 ; move Carry bit into carry flag position
	or dl, byte ptr [cc_s+CF_C_B]
	add al, 0 ; trigger flags
	; Save flags NF+ZF+CF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+CF_C_B], dl
	add rsp, 28h
	ret
Daa_I_A ENDP

PUBLIC Orcc_M_A ; Op 1A - Or CC with immediate byte
Orcc_M_A PROC
PBYTE EQU 2
	sub rsp, 28h
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+PBYTE], al
	; Get CC
	call getcc_s
	; Or with postbyte (dl)
	or al, byte ptr [rsp+PBYTE]
	; Set CC
	movzx cx, al
	call setcc_s
	add rsp, 28h
	ret
Orcc_M_A ENDP

PUBLIC Andcc_M_A ; Op 1C - And CC with immediate byte
Andcc_M_A PROC
PBYTE EQU 2
	sub rsp, 28h
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+PBYTE], al
	; Get CC
	call getcc_s
	; And with postbyte (dl)
	and al, byte ptr [rsp+PBYTE]
	; Set CC
	movzx cx, al
	call setcc_s
	add rsp, 28h
	ret
Andcc_M_A ENDP

PUBLIC Sex_I_A ; Op 1D - Sign Extend W into Q
Sex_I_A PROC
	sub rsp, 28h
	movsx cx, byte ptr [q_s+BREG] ; B Reg Sign extend 8 bits into 16
	mov word ptr [q_s+BREG], cx
	; trigger and save flags
	add cx, 0 ;
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	add rsp, 28h
	ret
Sex_I_A ENDP

PUBLIC Bra_R_A ; Op 20 - Branch Always relative to PC
Bra_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
	add rsp, 28h
	ret
Bra_R_A ENDP

PUBLIC Brn_R_A ; Op 21 - Branch Never relative to PC
Brn_R_A PROC
	inc word ptr [pc_s]
	ret
Brn_R_A ENDP

PUBLIC Bhi_R_A ; Op 22 - Branch if Higher relative to PC
Bhi_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+CF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	jne bhiret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bhiret:
	add rsp, 28h
	ret
Bhi_R_A ENDP

PUBLIC Bls_R_A ; Op 23 - Branch if lower or same relative to PC
Bls_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+CF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	je blsret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
blsret:
	add rsp, 28h
	ret
Bls_R_A ENDP

PUBLIC Bhs_R_A ; Op 24 - Branch if higher or same relative to PC
Bhs_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+CF_C_B], 1
	jne bhsret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bhsret:
	add rsp, 28h
	ret
Bhs_R_A ENDP

PUBLIC Blo_R_A ; Op 25 - Branch if lower relative to PC
Blo_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+CF_C_B], 1
	je bloret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bloret:
	add rsp, 28h
	ret
Blo_R_A ENDP

PUBLIC Bne_R_A ; Op 26 - Branch if not equal relative to PC
Bne_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+ZF_C_B], 1
	jne bneret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bneret:
	add rsp, 28h
	ret
Bne_R_A ENDP

PUBLIC Beq_R_A ; Op 27 - Branch if equal relative to PC
Beq_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+ZF_C_B], 1
	je beqret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
beqret:
	add rsp, 28h
	ret
Beq_R_A ENDP

PUBLIC Bvc_R_A ; Op 28 - Branch if overflow clear relative to PC
Bvc_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+VF_C_B], 1
	jne bvcret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bvcret:
	add rsp, 28h
	ret
Bvc_R_A ENDP

PUBLIC Bvs_R_A ; Op 29 - Branch if overflow set relative to PC
Bvs_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+VF_C_B], 1
	je bvsret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bvsret:
	add rsp, 28h
	ret
Bvs_R_A ENDP

PUBLIC Bpl_R_A ; Op 2A - Branch if plus relative to PC
Bpl_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+NF_C_B], 1
	jne bplret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bplret:
	add rsp, 28h
	ret
Bpl_R_A ENDP

PUBLIC Bmi_R_A ; Op 2B - Branch if minus relative to PC
Bmi_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	test byte ptr [cc_s+NF_C_B], 1
	je bmiret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bmiret:
	add rsp, 28h
	ret
Bmi_R_A ENDP

PUBLIC Bge_R_A ; Op 2C - Branch if greater and equal relative to PC
Bge_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	jne bgeret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bgeret:
	add rsp, 28h
	ret
Bge_R_A ENDP

PUBLIC Blt_R_A ; Op 2D - Branch if less than relative to PC
Blt_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	je bltret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bltret:
	add rsp, 28h
	ret
Blt_R_A ENDP

PUBLIC Bgt_R_A ; Op 2E - Branch if greater relative to PC
Bgt_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	jne bgtret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bgtret:
	add rsp, 28h
	ret
Bgt_R_A ENDP

PUBLIC Ble_R_A ; Op 2F - Branch if less or equal than relative to PC
Ble_R_A PROC
	sub rsp, 28h
	; Get Post Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	mov dl, byte ptr [cc_s+NF_C_B]
	xor dl, byte ptr [cc_s+VF_C_B]
	or dl, byte ptr [cc_s+ZF_C_B]
	je bltret
	call MemRead8_s
	movsx ax, al
	add word ptr [pc_s], ax
bltret:
	add rsp, 28h
	ret
Ble_R_A ENDP

PUBLIC Leax_X_A ; Op 30 - Load Effect Address into X
Leax_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov word ptr [x_s], ax
	add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	add rsp, 28h
	ret
Leax_X_A ENDP

PUBLIC Leay_X_A ; Op 31 - Load Effect Address into Y
Leay_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov word ptr [y_s], ax
	add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	add rsp, 28h
	ret
Leay_X_A ENDP

PUBLIC Leas_X_A ; Op 32 - Load Effect Address into S
Leas_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov word ptr [s_s], ax
	add rsp, 28h
	ret
Leas_X_A ENDP

PUBLIC Leau_X_A ; Op 33 - Load Effect Address into U
Leau_X_A PROC
	sub rsp, 28h
	call CalcEA_A
	mov word ptr [u_s], ax
	add rsp, 28h
	ret
Leau_X_A ENDP

PUBLIC Rts_I_A ; Op 39 - Return from subroutine
Rts_I_A PROC
	sub rsp, 28h
	; Get next stack byte and store it in PC MSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+MSB], al
	; Get next stack byte and store it in PC LSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+LSB], al
	add rsp, 28h
	ret
Rts_I_A ENDP

PUBLIC Abx_I_A ; Op 3A - Add unsigned B to X
Abx_I_A PROC
	movzx cx, byte ptr [q_s+BREG]
	add word ptr [x_s], cx
	ret
Abx_I_A ENDP

PUBLIC Rti_I_A ; Op 3B - Return from Interupt
Rti_I_A PROC
	sub rsp, 26h
	push bx
	; Set CC from stack
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	movzx cx, al
	mov bx, cx ; remember ccbits
	call setcc_s
	; Clear InInterupt
	mov byte ptr [InInterupt_s], 0
	; if EF is set restore all registers
	test bx, EF_C
	je rti2
	; pul a
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+AREG], al
	; pul b
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+BREG], al
	; If MD native bit set pul e & f
	test byte ptr [mdbits_s], MD_NATIVE
	je rti1
	; pul e
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	; pul f
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
rti1:
	; pul dp
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [dp_s+MSB], al
	; pul x MSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [x_s+MSB], al
	; pul x LSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [x_s+LSB], al
	; pul y MSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [y_s+MSB], al
	; pul y LSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [y_s+LSB], al
	; pul u MSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [u_s+MSB], al
	; pul u LSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [u_s+LSB], al
rti2:
	; pul pc MSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+MSB], al
	; pul pc LSB
	mov cx, word ptr [s_s]
	inc word ptr [s_s]
	call MemRead8_s
	mov byte ptr [pc_s+LSB], al
	pop bx
	add rsp, 26h
	ret
Rti_I_A ENDP

PUBLIC Cwai_I_A ; Op 3C - Clear CC Bits and Wait For Interrupt
Cwai_I_A PROC
	sub rsp, 28h
	; Get Immediate value at PC++
	movzx rcx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx,ax
	; Get CC
	call getcc_s
	; And with postbyte (dl)
	and al,dl
	; Set CC
	movzx cx,al
	call setcc_s
	mov eax, dword ptr [gCycleFor]
	mov dword ptr [CycleCounter], eax
	mov byte ptr [SyncWaiting_s], 1
	add rsp, 28h
	ret
Cwai_I_A ENDP

PUBLIC Mul_I_A ; Op 3D - D = A * B
Mul_I_A PROC
	mov al, byte ptr [q_s+AREG]
	mul byte ptr [q_s+BREG]
	mov word ptr [q_s+DREG], ax
	add ax, 0h ; trigger ZF
	bt ax, 7 ; trigger CF depending on bit 7
	; Save flags NF+ZF+VF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	ret
Mul_I_A ENDP

PUBLIC Swi1_I_A ; Op 3F - Software wait for interupt 1
Swi1_I_A PROC
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
	je swi11
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
swi11:
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
	; PC = VSWI
	mov cx, VSWI
	call MemRead16_s
	mov word ptr [pc_s], ax
	; IF = 1
	mov byte ptr [cc_s+IF_C_B], 1
	; FF = 1
	mov byte ptr [cc_s+FF_C_B], 1
	add rsp, 28h
	ret
Swi1_I_A ENDP

PUBLIC Nega_I_A ; Op 40 - Negate A reg
Nega_I_A PROC
	; Negate A Reg and save the FLAGS
	neg byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Nega_I_A ENDP

PUBLIC Coma_I_A ; Op 43 - Complement A reg
Coma_I_A PROC
	; Negate A Reg and save the FLAGS
	not byte ptr [q_s+AREG]
	add byte ptr [q_s+AREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 1
	ret
Coma_I_A ENDP

PUBLIC Lsra_I_A ; Op 44 - Logical Shift Right A reg
Lsra_I_A PROC
	; Shift Right A Reg and save the FLAGS
	shr byte ptr [q_s+AREG], 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	ret
Lsra_I_A ENDP

PUBLIC Rora_I_A ; Op 46 - Rotate Right through Carry A reg
Rora_I_A PROC
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right A Reg and save the FLAGS
	rcr byte ptr [q_s+AREG], 1
	; rcr only affects Carry.  Save the Carry first in dx then
	; add 0 to result to trigger Zero and Sign/Neg flags
	setc byte ptr [cc_s+CF_C_B]
	add byte ptr [q_s+AREG], 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rora_I_A ENDP

PUBLIC Asra_I_A ; Op 47 - Arithmetic Shift Right A reg
Asra_I_A PROC
	; Rotate Right A Reg and save the FLAGS
	sar byte ptr [q_s+AREG], 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Asra_I_A ENDP

PUBLIC Asla_I_A ; Op 48 - Arithmetic Shift Left A reg
Asla_I_A PROC
	; Shift left A Reg and save the FLAGS
	sal byte ptr [q_s+AREG], 1
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Asla_I_A ENDP

PUBLIC Rola_I_A ; Op 49 - Rotate Left through carry A reg
Rola_I_A PROC
	mov al, byte ptr [q_s+AREG]
	; Save the top bit for the V flag calc
	mov dl, al
	and dl, 080h
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right A Reg and save the FLAGS
	rcl al, 1
	mov byte ptr [q_s+AREG], al
	; rcl only affects Carry.  Save the Carry first in dx then
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dl, al
	shr dl, 7
	mov byte ptr [cc_s+VF_C_B], dl
	; add 0 to result to trigger Zero and Sign/Neg flags
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rola_I_A ENDP

PUBLIC Deca_I_A ; Op 4A - Decrement A Reg 
Deca_I_A PROC
	; Decrement the A Reg and save the FLAGS
	dec byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Deca_I_A ENDP

PUBLIC Inca_I_A ; Op 4C - Increment A Reg
Inca_I_A PROC
	Inc byte ptr [q_s+AREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Inca_I_A ENDP

PUBLIC Tsta_I_A ; Op 4D - Test A Reg
Tsta_I_A PROC
	; Add A Reg with 0
	add byte ptr [q_s+AREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Tsta_I_A ENDP

PUBLIC Clra_I_A ; Op 4F - Clear A Reg
Clra_I_A PROC
	mov byte ptr [q_s+AREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clra_I_A ENDP

PUBLIC Negb_I_A ; Op 50 - Negate B reg
Negb_I_A PROC
	; Negate B Reg and save the FLAGS
	neg byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Negb_I_A ENDP

PUBLIC Comb_I_A ; Op 53 - Complement B reg
Comb_I_A PROC
	; Negate B Reg and save the FLAGS
	not byte ptr [q_s+BREG]
	add byte ptr [q_s+BREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 1 ; Always Set C flag
	ret
Comb_I_A ENDP

PUBLIC Lsrb_I_A ; Op 54 - Logical Shift Right B reg
Lsrb_I_A PROC
	; Shift Right B Reg and save the FLAGS
	shr byte ptr [q_s+BREG], 1
	; Save flags NF+ZF+CF
	setz byte ptr [cc_s+ZF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	ret
Lsrb_I_A ENDP

PUBLIC Rorb_I_A ; Op 56 - Rotate Right through Carry B reg
Rorb_I_A PROC
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right B Reg and save the FLAGS
	rcr byte ptr [q_s+BREG], 1
	; rcr only affects Carry.  Save the Carry first in dx then
	; add 0 to result to trigger Zero and Sign/Neg flags
	setc byte ptr [cc_s+CF_C_B]
	add byte ptr [q_s+BREG], 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rorb_I_A ENDP

PUBLIC Asrb_I_A ; Op 57 - Arithmetic Shift Right B reg
Asrb_I_A PROC
	; Rotate Right B Reg and save the FLAGS
	sar byte ptr [q_s+BREG], 1
	; Save flags NF+ZF+CF
	setz byte ptr [cc_s+ZF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Asrb_I_A ENDP

PUBLIC Aslb_I_A ; Op 48 - Arithmetic Shift Left B reg
Aslb_I_A PROC
	; Shift left B Reg and save the FLAGS
	sal byte ptr [q_s+BREG], 1
	; Save flags NF+ZF+CF+VF
	setz byte ptr [cc_s+ZF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	ret
Aslb_I_A ENDP

PUBLIC Rolb_I_A ; Op 59 - Rotate Left through carry B reg
Rolb_I_A PROC
	mov al, byte ptr [q_s+BREG]
	; Save the top bit for the V flag calc
	mov dl, al
	and dl, 080h
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right A Reg and save the FLAGS
	rcl al, 1
	mov byte ptr [q_s+BREG], al
	; rcl only affects Carry.  Save the Carry first in dx then
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dl, al
	shr dl, 7
	mov byte ptr [cc_s+VF_C_B], dl
	; add 0 to result to trigger Zero and Sign/Neg flags
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Rolb_I_A ENDP

PUBLIC Decb_I_A ; Op 5A - Decrement B Reg 
Decb_I_A PROC
	; Decrement the B Reg and save the FLAGS
	dec byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	ret
Decb_I_A ENDP

PUBLIC Incb_I_A ; Op 5C - Increment B Reg
Incb_I_A PROC
	Inc byte ptr [q_s+BREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	ret
Incb_I_A ENDP

PUBLIC Tstb_I_A ; Op 5D - Test B Reg
Tstb_I_A PROC
	; Add B Reg with 0
	add byte ptr [q_s+BREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	ret
Tstb_I_A ENDP

PUBLIC Clrb_I_A ; Op 5F - Clear B Reg
Clrb_I_A PROC
	mov byte ptr [q_s+BREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clrb_I_A ENDP

PUBLIC Neg_X_A ; Op 60 - Negate byte Memory Indexed
Neg_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Negate the byte and save the FLAGS
	neg al
	; Save flags NF+ZF+CF+VF
	setz byte ptr [cc_s+ZF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write negated byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Neg_X_A ENDP

PUBLIC Oim_X_A ; Op 61 - Logical OR Indexed memory byte with immediate value
Oim_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov word ptr [rsp+VAL], ax
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Or Mem Byte with saved Immediate Value
	or al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Write Or'd byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Oim_X_A ENDP

PUBLIC Aim_X_A ; Op 62 - Logical AND Indexed memory byte with immediate value
Aim_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov word ptr [rsp+VAL], ax
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B],0
	; Write Or'd byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Aim_X_A ENDP

PUBLIC Com_X_A ; Op 63 - Complement Indexed memory byte
Com_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Negate the byte and save the FLAGS
	not al
	add al, 0 ; x86 not doesn't affect any flags so trigger
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 1 ; always set carry
	; Write complemented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Com_X_A ENDP

PUBLIC Lsr_X_A ; Op 64 - Logical Shift Right Indexed memory byte
Lsr_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Shift Right the byte and save the FLAGS
	shr al, 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Lsr_X_A ENDP

PUBLIC Eim_X_A ; Op 65 - Logical XOR Indexed memory byte with immediate value
Eim_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov word ptr [rsp+VAL], ax
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Xor Mem Byte with saved Immediate Value
	xor byte ptr [rsp+VAL], al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	sets byte ptr [cc_s+NF_C_B]
	; Write Xor'd byte (on stack) to saved address (on stack)
	mov cx, word ptr [rsp+VAL]
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s  ; dx - address, cx - byte
	add rsp, 28h
	ret
Eim_X_A ENDP

PUBLIC Ror_X_A ; Op 66 - Rotate Right through Carry Indexed memory byte
Ror_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right the byte and save the FLAGS
	rcr al, 1
	; rcr only affects Carry.  Save the Carry first in dx then
	; add 0 to result to trigger Zero and Sign/Neg flags
	setc byte ptr [cc_s+CF_C_B]
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Ror_X_A ENDP

PUBLIC Asr_X_A ; Op 67 - Arithmetic Shift Right Indexed memory byte
Asr_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Rotate Right the byte and save the FLAGS
	sar al, 1
	; Save flags NF+ZF+CF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asr_X_A ENDP

PUBLIC Asl_X_A ; Op 68 - Arithmetic Shift Left Indexed memory byte
Asl_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Shift left the byte and save the FLAGS
	sal al, 1
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asl_X_A ENDP

PUBLIC Rol_X_A ; Op 69 - Rotate Left through Carry Indexed memory byte
Rol_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Save the top bit for the V flag calc
	mov dl, al
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right the byte and save the FLAGS
	rcl al, 1
	;mov word ptr [rsp+VAL], ax
	; rcl only affects Carry.  Save the Carry first in dx then
	setc byte ptr [cc_s+CF_C_B]
	xor dl, al
	shr dl, 7 ; mov V bit into correct position
	mov byte ptr [cc_s+VF_C_B], dl
	; add 0 to result to trigger Zero and Sign/Neg flags
	;mov ax, [rsp+VAL] ; get result from stack
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Rol_X_A ENDP

PUBLIC Dec_X_A ; Op 6A - Decrement Indexed memory byte
Dec_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Decrement the A Reg and save the FLAGS
	dec al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Dec_X_A ENDP

PUBLIC Tim_X_A ; Op 6B - Test Indexed memory byte with immediate value
Tim_X_A PROC
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, ax
	mov word ptr [rsp+VAL], ax
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	sets byte ptr [cc_s+NF_C_B]
	add rsp, 28h
	ret
Tim_X_A ENDP

PUBLIC Inc_X_A ; Op 6C - Increment Indexed memory byte
Inc_X_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Increment the byte and save the FLAGS
	Inc al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Inc_X_A ENDP

PUBLIC Tst_X_A ; Op 6D - Test Indexed memory byte
Tst_X_A PROC
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Save Address and use it to get a mem byte
	mov cx, ax
	call MemRead8_s
	; Add Mem Byte with 0
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	sets byte ptr [cc_s+NF_C_B]
	add rsp, 28h
	ret
Tst_X_A ENDP

PUBLIC Jmp_X_A ; Op 6E - Jump to Indexed memory address
Jmp_X_A PROC
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Update PC_S with Address
	mov word ptr [pc_s], ax
	add rsp, 28h
	ret
Jmp_X_A ENDP

PUBLIC Clr_X_A ; Op 6F - Clear Index memory byte
Clr_X_A PROC
	sub rsp, 28h
	; Calc Address from two bytes at 
	call CalcEA_A
	; Use Address to clear byte
	mov dx, ax
	mov cx, 0
	call MemWrite8_s ; dx - address, cx - byte
	; Save flags NF+ZF+VF+CF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	add rsp, 28h
	ret
Clr_X_A ENDP

PUBLIC Neg_E_A ; Op 70 - Negate byte from Extended Memory
Neg_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Negate the byte and save the FLAGS
	neg al
	; Save flags NF+ZF+CF+VF
	setz byte ptr [cc_s+ZF_C_B]
	setc byte ptr [cc_s+CF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write negated byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Neg_E_A ENDP

PUBLIC Oim_E_A ; Op 71 - Logical OR Extended memory byte with immediate value
Oim_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Or Mem Byte with saved Immediate Value
	or al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; Write Or'd byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Oim_E_A ENDP

PUBLIC Aim_E_A ; Op 72 - Logical AND Extended memory byte with immediate value
Aim_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B],0
	; Write Or'd byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Aim_E_A ENDP

PUBLIC Com_E_A ; Op 73 - Complement Extended memory byte
Com_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Negate the byte and save the FLAGS
	not al
	add al, 0 ; x86 not doesn't affect any flags so trigger
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 1 ; Always Set C flag
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Com_E_A ENDP

PUBLIC Lsr_E_A ; Op 74 - Logical Shift Right Extended memory byte
Lsr_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Shift Right the byte and save the FLAGS
	shr al, 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+NF_C_B], 0
	; Write shifted byte (al) to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Lsr_E_A ENDP

PUBLIC Eim_E_A ; Op 75 - Logical XOR Extended memory byte with immediate value
Eim_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Xor Mem Byte with saved Immediate Value
	xor al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	mov byte ptr [cc_s+VF_C_B], 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write Xor'd byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s  ; dx - address, cx - byte
	add rsp, 28h
	ret
Eim_E_A ENDP

PUBLIC Ror_E_A ; Op 76 - Rotate Right through Carry Extended memory byte
Ror_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Restore Carry Flag
	bt word ptr [cc_s+CF_C_B], 0
	; Rotate Right the byte and save the FLAGS
	rcr al, 1
	; rcr only affects Carry.  Save the Carry flag first
	setc byte ptr [cc_s+CF_C_B]
	; add 0 to result to trigger Zero and Sign/Neg flags
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte to address (saved on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Ror_E_A ENDP

PUBLIC Asr_E_A ; Op 77 - Arithmetic Shift Right Extended memory byte
Asr_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Rotate Right the byte and save the FLAGS
	sar al, 1
	; Save flags NF+ZF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asr_E_A ENDP

PUBLIC Asl_E_A ; Op 78 - Arithmetic Shift Left Extended memory byte
Asl_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Shift left the byte and save the FLAGS
	sal al, 1
	; Save flags NF+ZF+CF+VF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write shifted byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s
	add rsp, 28h
	ret
Asl_E_A ENDP

PUBLIC Rol_E_A ; Op 79 - Rotate Left through Carry Extended memory byte
Rol_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Save the top bit for the V flag calc
	mov dl, al
	; Restore CF Flag
	bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
	; Rotate Right the byte and save the FLAGS
	rcl al, 1
	; rcl only affects Carry.  Save the Carry first
	setc byte ptr [cc_s+CF_C_B]
	; Xor the new top bit with the previous top bit in dx to calc V flag
	xor dl, al
	shr dl, 7
	mov byte ptr [cc_s+VF_C_B], dl
	add al, 0 ; trigger NZ flags
	; Save flags NF+ZF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	; Write rotated byte (ah) to address (saved on stack)
	movzx cx, al
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Rol_E_A ENDP

PUBLIC Dec_E_A ; Op 7A - Decrement Extended memory byte
Dec_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Decrement the A Reg and save the FLAGS
	dec al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Dec_E_A ENDP

PUBLIC Tim_E_A ; Op 7B - Test Extended memory byte with immediate value
Tim_E_A PROC
PBYTE EQU 2
	sub rsp, 28h
	; Get & Save Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [rsp+VAL], al
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; And Mem Byte with saved Immediate Value
	and al, byte ptr [rsp+VAL]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Tim_E_A ENDP

PUBLIC Inc_E_A ; Op 7C - Increment Extended memory byte
Inc_E_A PROC
MEMADR EQU 2
VAL EQU 4
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Increment the byte and save the FLAGS
	Inc al
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	; Write decremented byte (on stack) to saved address (on stack)
	mov cx, ax
	mov dx, word ptr [rsp+MEMADR]
	call MemWrite8_s ; dx - address, cx - byte
	add rsp, 28h
	ret
Inc_E_A ENDP

PUBLIC Tst_E_A ; Op 7D - Test Extended memory byte
Tst_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Save Address and use it to get a mem byte
	mov word ptr [rsp+MEMADR], ax
	mov cx, ax
	call MemRead8_s
	; Add Mem Byte with 0
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov  byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Tst_E_A ENDP

PUBLIC Jmp_E_A ; Op 7E - Jump to Extended memory address
Jmp_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	; Update PC_S with Address
	mov word ptr [pc_s], ax
	add rsp, 28h
	ret
Jmp_E_A ENDP

PUBLIC Clr_E_A ; Op 7F - Clear Extended memory byte
Clr_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov dx, ax
	; Use Address to clear byte
	mov cx, 0
	call MemWrite8_s ; dx - address, cx - byte
	; Save flags NF+ZF+VF+CF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	add rsp, 28h
	ret
Clr_E_A ENDP
END