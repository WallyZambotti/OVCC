INCLUDE hd6309asm_h.asm

.data

.code

PUBLIC CalcEA_A ; Calculate Effect Address from post byte pc
CalcEA_A PROC ; CalcEA_a()
SOURCE EQU 2
DEST EQU 4
	push rbx
	sub rsp, 20h
	; get the instruction post byte and save it
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx ecx, al
	movzx rax, cl ; Save post byte
	movzx rbx, cl ; Save post byte
	movzx r8, cl ; Save it again
	; register = ((posybyte >> 5) & 3) + 1
	shr al, 5
	and al, 3
	inc al
	bt bx, 7; If bit 7 is not set
	jnc ceaelse1
	; switch (posybtye & 0x1f)
	and bl, 1FH
	lea rdx, ceaswtch1
	mov rdx, [rdx+rbx*8]
	jmp rdx
ceaswtch1:
	QWORD ceacase100
	QWORD ceacase101
	QWORD ceacase102
	QWORD ceacase103
	QWORD ceacase104
	QWORD ceacase105
	QWORD ceacase106
	QWORD ceacase107
	QWORD ceacase108
	QWORD ceacase109
	QWORD ceacase10A
	QWORD ceacase10B
	QWORD ceacase10C
	QWORD ceacase10D
	QWORD ceacase10E
	QWORD ceacase10F
	QWORD ceacase110
	QWORD ceacase111
	QWORD cearet
	QWORD ceacase113
	QWORD ceacase114
	QWORD ceacase115
	QWORD ceacase116
	QWORD ceacase117
	QWORD ceacase118
	QWORD ceacase119
	QWORD ceacase11A
	QWORD ceacase11B
	QWORD ceacase11C
	QWORD ceacase11D
	QWORD ceacase11E
	QWORD ceacase11F
ceacase100: ; Post-inc by 1
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rax, word ptr [rdx]
;	(*xfreg16_s[Register])++;
	inc word ptr [rdx]
	add dword ptr [CycleCounter], 2
	jmp cearet
ceacase101: ; Post-inc by 2
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rax, word ptr [rdx]
;	(*xfreg16_s[Register])+=2;
	add word ptr [rdx], 2
	add dword ptr [CycleCounter], 3
	jmp cearet
ceacase102: ; Pre-dec by 1
;	ea = (*xfreg16_s[Register]);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
;	(*xfreg16_s[Register])-=1;
	dec word ptr [rcx]
	movzx rax, word ptr [rcx]
	add dword ptr [CycleCounter], 2
	jmp cearet
ceacase103: ; Pre-dec by 2
;	ea = (*xfreg16_s[Register]);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
;	(*xfreg16_s[Register])-=2;
	sub word ptr [rcx], 2
	movzx rax, word ptr [rcx]
	add dword ptr [CycleCounter], 3
	jmp cearet
ceacase104: ; No offset
;	ea = (*xfreg16_s[Register]);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	jmp cearet
ceacase105: ; B.reg offset
;	ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswlsb);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	movsx dx, byte ptr [q_s+BREG]
	add ax, dx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase106: ; A.reg offset
;	ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswmsb);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	movsx dx, byte ptr [q_s+AREG]
	add ax, dx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase107: ; E.reg offset
;	ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswmsb);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	movsx dx, byte ptr [q_s+EREG]
	add ax, dx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase108: ; 8 bit offset
;	ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
	movzx rbx, ax
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movsx ax, al
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rbx*8]
	movzx rcx, word ptr [rcx]
	add ax, cx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase109: ; 16 bit offset
;	ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
	movzx rbx, ax
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rbx*8]
	movzx rcx, word ptr [rcx]
	add ax, cx
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase10A: ; F.reg offset
;	ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswlsb);
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	movsx dx, byte ptr [q_s+FREG]
	add ax, dx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase10B: ; D.reg offset
;	ea = (*xfreg16_s[Register]) + q_s.Word.lsw;
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	mov dx, word ptr [q_s+DREG]
	add ax, dx
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase10C: ; 8 bit PC relative
;	ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [pc_s]
	movsx ax, al
	add ax, cx
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase10D: ; 16 bit PC relative
;	ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 2;
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	movzx rcx, word ptr [pc_s]
	add ax, cx
	add dword ptr [CycleCounter], 5
	jmp cearet
ceacase10E: ; W.reg offset
;	ea = (*xfreg16_s[Register]) + q_s.Word.msw;
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	mov dx, word ptr [q_s+WREG]
	add ax, dx
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase10F: ; W.reg
	; switch((postbyte >> 5) & 3)
	shr r8, 5
	and r8, 3
	lea rdx, ceaswtch2
	mov rdx, [rdx+r8*8]
	jmp rdx
ceaswtch2:
	QWORD ceacase200
	QWORD ceacase201
	QWORD ceacase202
	QWORD ceacase203
ceacase200: ; No offset from W.reg
	movzx rax, word ptr [q_s+WREG]
	jmp cearet
ceacase201: ; 16 bit offset from W.reg
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	movzx rcx, word ptr [q_s+WREG]
	add ax, cx
	add dword ptr [CycleCounter], 2
	jmp cearet
ceacase202: ; Post-inc by 2 from W.reg
	movzx rax, word ptr [q_s+WREG]
	add word ptr [q_s+WREG], 2
	add dword ptr [CycleCounter], 1
	jmp cearet
ceacase203: ; Pre-dec by 2 from W.reg
	sub word ptr [q_s+WREG], 2
	movzx rax, word ptr [q_s+WREG]
	add dword ptr [CycleCounter], 1
	jmp cearet
;   end switch 2
ceacase110: ; W.reg
	; switch((postbyte >> 5) & 3)
	shr r8, 5
	and r8, 3
	lea rdx, ceaswtch3
	mov rdx, [rdx+r8*8]
	jmp rdx
ceaswtch3:
	QWORD ceacase300
	QWORD ceacase301
	QWORD ceacase302
	QWORD ceacase303
ceacase300: ; Indirect no offset from W.reg
	movzx rcx, word ptr [q_s+WREG]
	call MemRead16_s
	add dword ptr [CycleCounter], 3
	jmp cearet
ceacase301: ; Indirect 16 bit offset from W.reg
	; Get offset Word at PC++
	movzx rcx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	add ax, word ptr [q_s+WREG]
	mov cx, ax
	call MemRead16_s
	add dword ptr [CycleCounter], 5
	jmp cearet
ceacase302: ; Indirect post-inc by 2 from W.reg
	mov cx, word ptr [q_s+WREG]
	add word ptr [q_s+WREG], 2
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase303: ;  Indirect pre-dec by 2 from W.reg
	sub word ptr [q_s+WREG], 2
	mov cx, word ptr [q_s+WREG]
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
	; end switch 3
ceacase111: ; Indirect post-inc by 2
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	(*xfreg16_s[Register])+=2;
	add word ptr [rdx], 2
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 6
	jmp cearet
ceacase113: ; Indirect pre-dec by 2
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
;	(*xfreg16_s[Register])-=2;
	sub word ptr [rdx], 2
	movzx rcx, word ptr [rdx]
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 6
	jmp cearet
ceacase114: ; Indirect no offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 3
	jmp cearet
ceacase115: ; Indirect B.reg offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea += (signed char)q_s.Byte.lswlsb
	movsx dx, byte ptr [q_s+BREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase116: ; Indirect A.reg offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea += (signed char)q_s.Byte.lswmsb
	movsx dx, byte ptr [q_s+AREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase117: ; Indirect E.reg offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea += (signed char)q_s.Byte.mswmsb
	movsx dx, byte ptr [q_s+EREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase118: ; Indirect 8 bit offset
;	ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
	movzx rbx, ax
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movsx ax, al
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rbx*8]
	movzx rcx, word ptr [rcx]
	add cx, ax
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase119: ; 16 bit offset
;	ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
	movzx rbx, ax
	; Get offset Byte at PC++
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rbx*8]
	movzx rcx, word ptr [rcx]
	add cx, ax
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 7
	jmp cearet
ceacase11A: ; Indirect F.reg offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea += (signed char)q_s.Byte.mswlsb
	movsx dx, byte ptr [q_s+FREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase11B: ; Indirect D.reg offset
;	ea = (*xfreg16_s[Register]);
	lea rdx, word ptr [xfreg16_s]
	mov rdx, qword ptr [rdx+rax*8]
	movzx rcx, word ptr [rdx]
;	ea += (signed char)q_s.Word.lsw
	mov dx, word ptr [q_s+DREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 7
	jmp cearet
ceacase11C: ; Indirect 8 bit PC relative
;	ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	movzx rcx, word ptr [pc_s]
	movsx ax, al
	add cx, ax
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 4
	jmp cearet
ceacase11D: ; Indirect 16 bit PC relative
;	ea = (signed short)pc_s.Reg + MemRead16_s(pc_s.Reg) + 2;
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	movzx rcx, word ptr [pc_s]
	add cx, ax
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 8
	jmp cearet
ceacase11E: ; Indirect W.reg offset
;	ea = (*xfreg16_s[Register]) + q_s.Word.msw;
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rcx, word ptr [rcx]
	mov dx, word ptr [q_s+WREG]
	add cx, dx
;	ea = MemRead16(ea)
	call MemRead16_s
	add dword ptr [CycleCounter], 7
	jmp cearet
ceacase11F: ; Indirect extended
;	ea = MemRead16_s(pc_s.Reg);
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	movzx rcx, ax
;	ea = MemRead16_s(ea)
	call MemRead16_s
	jmp cearet
	add dword ptr [CycleCounter], 8
	; end switch 1
ceaelse1: ; 5 bit offset
;	ea = (*xfreg16_s[Register]) + (signed)postbyte[bits 4-0];
	lea rcx, word ptr [xfreg16_s]
	mov rcx, qword ptr [rcx+rax*8]
	movzx rax, word ptr [rcx]
	mov rdx, r8
	shl dx, 11
	sar dx, 11
	add ax, dx
	add dword ptr [CycleCounter], 1
cearet:
	add rsp, 20h
	pop rbx
	ret
CalcEA_A ENDP
END