; X86Flags : C-1, H-10 Z-40, N-80, V-800

INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC
EXTERN	CalcEA_A: PROC

.data

.code

PUBLIC Band_A ; Op 1130 - Bit And Register with Memory Bit
Band_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) == 0
  bt ax, cx
  jc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] &= ~(1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  btr cx, dx
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() &= ~(1 << Dest));
  call getcc_s
  mov cx, ax
  btr cx, dx
  call setcc_s
exit:
  pop rbx
	add rsp, 20h
	ret
Band_A ENDP

PUBLIC Biand_A ; Op 1131 - Bit And Register with Inverted Memory Bit
Biand_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) != 0
  bt ax, cx
  jnc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] &= ~(1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  btr cx, dx
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() &= ~(1 << Dest));
  call getcc_s
  mov cx, ax
  btr cx, dx
  call setcc_s
exit:
  pop rbx
	add rsp, 20h
	ret
Biand_A ENDP

PUBLIC Bor_A ; Op 1132 - Bit Or Register with Memory Bit
Bor_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) != 0
  bt ax, cx
  jnc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] |= (1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  bts cx, dx
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() | (1 << Dest));
  call getcc_s
  mov cx, ax
  bts cx, dx
  call setcc_s
exit:
  pop rbx
	add rsp, 20h
	ret
Bor_A ENDP

PUBLIC Bior_A ; Op 1133 - Bit Or Register with Inverted Memory Bit
Bior_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) == 0
  bt ax, cx
  jc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] |= (1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  bts cx, dx
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() | (1 << Dest));
  call getcc_s
  mov cx, ax
  bts cx, dx
  call setcc_s
exit:
  pop rbx
  add rsp, 20h
  ret
Bior_A ENDP

PUBLIC Beor_A ; Op 1134 - Bit Xor Register with Memory Bit
Beor_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) != 0
  bt ax, cx
  jnc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] ^= (1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  bt cx, dx
  jc clrbit
  bts cx, dx
  jmp save
clrbit:
  btr cx, dx
save:
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() ^ (1 << Dest));
  call getcc_s
  mov cx, ax
  bt cx, dx
  jc clrccbit
  bts cx, dx
  jmp savecc
clrccbit:
  btr cx, dx
savecc:
  call setcc_s
exit:
  pop rbx
  add rsp, 20h
  ret
Beor_A ENDP

PUBLIC Bieor_A ; Op 1135 - Bit Xor Register with Inverted Memory Bit
Bieor_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  ; if dp_byte & (1 << Source) == 0
  bt ax, cx
  jc exit
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; *ureg8[register] ^= (1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  bt cx, dx
  jc clrbit
  bts cx, dx
  jmp save
clrbit:
  btr cx, dx
save:
  mov byte ptr [rax], cl
  jmp exit
getcc:
  ; setcc(getcc() ^ (1 << Dest));
  call getcc_s
  mov cx, ax
  bt cx, dx
  jc clrccbit
  bts cx, dx
  jmp savecc
clrccbit:
  btr cx, dx
savecc:
  call setcc_s
exit:
  pop rbx
  add rsp, 20h
  ret
Bieor_A ENDP

PUBLIC Ldbt_A ; Op 1136 - Load Register Bit with Memory Bit
Ldbt_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if dp_byte & (1 << Source) != 0
  bt ax, cx
  jnc clrbit
  ; if register is CC need to getcc
  cmp bl, 2
  je setgetcc
  ; then *ureg8[register] |= (1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  bts cx, dx
  mov byte ptr [rax], cl
  jmp exit
setgetcc:
  ; setcc(getcc() | (1 << Dest));
  call getcc_s
  mov cx, ax
  bts cx, dx
  call setcc_s
  jmp exit
clrbit:
  ; if register is CC need to getcc
  cmp bl, 2
  je clrgetcc
  ; else *ureg8[register] &= ~(1 << Dest);
  lea rax, byte ptr [ureg8_s]
  mov rax, qword ptr [rax+rbx*8]
  movzx cx, byte ptr [rax]
  btr cx, dx
  mov byte ptr [rax], cl
  jmp exit
clrgetcc:
  ; setcc(getcc() | (1 << Dest));
  call getcc_s
  mov cx, ax
  btr cx, dx
  call setcc_s
exit:
  pop rbx
  add rsp, 20h
  ret
Ldbt_A ENDP

PUBLIC Stbt_A ; Op 1137 - Store Register Bit with Memory Bit
Stbt_A PROC
	sub rsp, 20h
  push rbx
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  movzx rbx, ax ; save postbyte
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
  mov r8, rcx ; save dp byte address for later rewrite. Assumes MemRead8_s is register r8 safe
	; And use it to get a mem byte
	call MemRead8_s ; al now contains dp byte
  mov cx, bx ; Source bit
  mov dx, bx ; Dest bit
  shr bl, 6 ; Register = postbyte >> 6;
  ; if register == 3 then raise invalid instruction and exit
  cmp bl, 3
  jne postbyteok
  call InvalidInsHandler_s
  jmp exit
postbyteok:
  shr cl, 3 ; Source = ((postbyte>>3) & 7) ;
  and cl, 7
  and dl, 7 ; Dest = (postbyte & 7) ;
  ; if register is CC need to getcc
  cmp bl, 2
  je getcc
  ; if *ureg8[register] & (1 << Source) != 0
  lea r9, byte ptr [ureg8_s]
  mov r9, qword ptr [r9+rbx*8]
  movzx bx, byte ptr [r9]
  jmp clrorset
getcc:
  mov r9, rax ; getcc_s destroys ax so save in R9. Assumes R9 is safe.
  call getcc_s
  mov bx, ax
  mov rax, r9
clrorset:
  bt bx, cx
  jnc clrbit
  ; then dp_byte |= (1 << Dest);
  bts ax, dx
  jmp save
clrbit:
  ; else dp_byte &= ~(1 << Dest);
  btr ax, dx
save:
  mov cx, ax
  mov rdx, r8
  call MemWrite8_s ; r8 contains saved dp byte address
exit:
  pop rbx
  add rsp, 20h
  ret
Stbt_A ENDP

PUBLIC Tfm1_A ; Op 1138 - Transfer Block Mem+ to Mem+
Tfm1_A PROC
  sub rsp, 28h
	; Test W Reg and exit if zero
	cmp word ptr [q_s+WREG],0
  jne doagain
  ; finish up
  inc word ptr [pc_s]
  jmp exit
doagain:
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  movzx rcx, ax ; Source Reg
  movzx rdx, ax ; Dest Reg
  shr cx, 4
  and dx, 0Fh
  ; if (Source > 4 || Dest > 4)
  cmp cx, 4
  jg abort
  cmp dx, 4
  jle cont
abort:
  call InvalidInshandler_s
  jmp exit
cont:
  ; temp8 = MemRead8(*xfreg16[Source]);
  lea r8, byte ptr [xfreg16_s]
  mov r9, r8
  mov r8, qword ptr [r8+rcx*8]
  mov cx, word ptr [r8]
  call MemRead8_s ; ax - byte to move
  ; MemWrite8(temp8, *xfreg16[Dest]);
  mov r9, qword ptr [r9+rdx*8]
  mov dx, word ptr [r9]
  mov cx, ax
  call MemWrite8_s
  inc word ptr [r8] ; (*xfreg16[Source])++;
  inc word ptr [r9] ; (*xfreg16[Dest])++;
  ; W_REG--;
  dec word ptr [q_s+WREG]
  sub word ptr [pc_s], 2
exit:
  add rsp, 28h
  ret
Tfm1_A ENDP

PUBLIC Tfm2_A ; Op 1139 - Transfer Block Mem- to Mem-
Tfm2_A PROC
  sub rsp, 28h
	; Test W Reg and exit if zero
	cmp word ptr [q_s+WREG],0
  jne doagain
  ; finish up
  inc word ptr [pc_s]
  jmp exit
doagain:
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  movzx rcx, ax ; Source Reg
  movzx rdx, ax ; Dest Reg
  shr cx, 4
  and dx, 0Fh
  ; if (Source > 4 || Dest > 4)
  cmp cx, 4
  jg abort
  cmp dx, 4
  jle cont
abort:
  call InvalidInshandler_s
  jmp exit
cont:
  ; temp8 = MemRead8(*xfreg16[Source]);
  lea r8, byte ptr [xfreg16_s]
  mov r9, r8
  mov r8, qword ptr [r8+rcx*8]
  mov cx, word ptr [r8]
  call MemRead8_s ; ax - byte to move
  ; MemWrite8(temp8, *xfreg16[Dest]);
  mov r9, qword ptr [r9+rdx*8]
  mov dx, word ptr [r9]
  mov cx, ax
  call MemWrite8_s
  dec word ptr [r8] ; (*xfreg16[Source])++;
  dec word ptr [r9] ; (*xfreg16[Dest])++;
  ; W_REG--;
  dec word ptr [q_s+WREG]
  sub word ptr [pc_s], 2
exit:
  add rsp, 28h
  ret
Tfm2_A ENDP

PUBLIC Tfm3_A ; Op 113A - Transfer Block Mem+ to Mem
Tfm3_A PROC
  sub rsp, 28h
	; Test W Reg and exit if zero
	cmp word ptr [q_s+WREG],0
  jne doagain
  ; finish up
  inc word ptr [pc_s]
  jmp exit
doagain:
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  movzx rcx, ax ; Source Reg
  movzx rdx, ax ; Dest Reg
  shr cx, 4
  and dx, 0Fh
  ; if (Source > 4 || Dest > 4)
  cmp cx, 4
  jg abort
  cmp dx, 4
  jle cont
abort:
  call InvalidInshandler_s
  jmp exit
cont:
  ; temp8 = MemRead8(*xfreg16[Source]);
  lea r8, byte ptr [xfreg16_s]
  mov r9, r8
  mov r8, qword ptr [r8+rcx*8]
  mov cx, word ptr [r8]
  call MemRead8_s ; ax - byte to move
  ; MemWrite8(temp8, *xfreg16[Dest]);
  mov r9, qword ptr [r9+rdx*8]
  mov dx, word ptr [r9]
  mov cx, ax
  call MemWrite8_s
  inc word ptr [r8] ; (*xfreg16[Source])++;
  ; W_REG--;
  dec word ptr [q_s+WREG]
  sub word ptr [pc_s], 2
exit:
  add rsp, 28h
  ret
Tfm3_A ENDP

PUBLIC Tfm4_A ; Op 113B - Transfer Block Mem to Mem+
Tfm4_A PROC
  sub rsp, 28h
	; Test W Reg and exit if zero
	cmp word ptr [q_s+WREG],0
  jne doagain
  ; finish up
  inc word ptr [pc_s]
  jmp exit
doagain:
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  movzx rcx, ax ; Source Reg
  movzx rdx, ax ; Dest Reg
  shr cx, 4
  and dx, 0Fh
  ; if (Source > 4 || Dest > 4)
  cmp cx, 4
  jg abort
  cmp dx, 4
  jle cont
abort:
  call InvalidInshandler_s
  jmp exit
cont:
  ; temp8 = MemRead8(*xfreg16[Source]);
  lea r8, byte ptr [xfreg16_s]
  mov r9, r8
  mov r8, qword ptr [r8+rcx*8]
  mov cx, word ptr [r8]
  call MemRead8_s ; ax - byte to move
  ; MemWrite8(temp8, *xfreg16[Dest]);
  mov r9, qword ptr [r9+rdx*8]
  mov dx, word ptr [r9]
  mov cx, ax
  call MemWrite8_s
  inc word ptr [r9] ; (*xfreg16[Dest])++;
  ; W_REG--;
  dec word ptr [q_s+WREG]
  sub word ptr [pc_s], 2
exit:
  add rsp, 28h
  ret
Tfm4_A ENDP

PUBLIC Bitmd_M_A ; Op 113C - Bit the MD register with immediate mem
Bitmd_M_A PROC
  sub rsp, 28h
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  and al, 0C0h ; mask away bottom 6 bits
  mov cl, byte ptr [mdbits_s]
  and cl, al
  setz byte ptr [cc_s+ZF_C_B]
  ; Clear the MD bits that were tested
  not al
  and cl, al
  mov byte ptr [mdbits_s], cl
  add rsp, 28h
  ret
Bitmd_M_A ENDP

PUBLIC Ldmd_M_A ; Op 113D - Load the MD register from immediate mem
Ldmd_M_A PROC
  sub rsp, 28h
	; Get Immediate value at PC++
	mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
	call MemRead8_s ; ax holds immediate byte
  and al, 03h ; mask away top 6 bits
  mov byte ptr [mdbits_s], al
	movsx cx, al ; trigger any emulator requirements
	call setmd_s ; trigger any emulator requirements
  add rsp, 28h
  ret
Ldmd_M_A ENDP

PUBLIC Swi3_I_A ; Op 113F - Software wait for interupt 3
Swi3_I_A PROC
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
	; PC = VSWI3
	mov cx, VSWI3
	call MemRead16_s
	mov word ptr [pc_s], ax
	add rsp, 28h
	ret
Swi3_I_A ENDP

PUBLIC Come_I_A ; Op 1143 - Complement E reg
Come_I_A PROC
	; Negate A Reg and save the FLAGS
	not byte ptr [q_s+EREG]
	add byte ptr [q_s+EREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+CF_C_B], 1
	ret
Come_I_A ENDP

PUBLIC Dece_I_A ; Op 114A - Decrement E Reg 
Dece_I_A PROC
	; Decrement the A Reg and save the FLAGS
	dec byte ptr [q_s+EREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Dece_I_A ENDP

PUBLIC Ince_I_A ; Op 114C - Increment E Reg
Ince_I_A PROC
	Inc byte ptr [q_s+EREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Ince_I_A ENDP

PUBLIC Tste_I_A ; Op 114D - Test E Reg
Tste_I_A PROC
	; Add A Reg with 0
	add byte ptr [q_s+EREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Tste_I_A ENDP

PUBLIC Clre_I_A ; Op 114F - Clear E Reg
Clre_I_A PROC
	mov byte ptr [q_s+EREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clre_I_A ENDP

PUBLIC Comf_I_A ; Op 1153 - Complement F reg
Comf_I_A PROC
	; Negate A Reg and save the FLAGS
	not byte ptr [q_s+FREG]
	add byte ptr [q_s+FREG], 0 ; trigger flags
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+CF_C_B], 1
	ret
Comf_I_A ENDP

PUBLIC Decf_I_A ; Op 115A - Decrement F Reg 
Decf_I_A PROC
	; Decrement the A Reg and save the FLAGS
	dec byte ptr [q_s+FREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Decf_I_A ENDP

PUBLIC Incf_I_A ; Op 115C - Increment F Reg
Incf_I_A PROC
	Inc byte ptr [q_s+FREG]
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Incf_I_A ENDP

PUBLIC Tstf_I_A ; Op 115D - Test F Reg
Tstf_I_A PROC
	; Add A Reg with 0
	add byte ptr [q_s+FREG], 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	ret
Tstf_I_A ENDP

PUBLIC Clrf_I_A ; Op 115F - Clear F Reg
Clrf_I_A PROC
	mov byte ptr [q_s+FREG], 0
	; Save flags NF+ZF+CF+VF
	mov byte ptr [cc_s+ZF_C_B], 1
	mov byte ptr [cc_s+NF_C_B], 0
	mov byte ptr [cc_s+VF_C_B], 0
	mov byte ptr [cc_s+CF_C_B], 0
	ret
Clrf_I_A ENDP

PUBLIC Sube_M_A ; Op 1180 - Subtract Immediate Memory from Reg E
Sube_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	sub byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sube_M_A ENDP

PUBLIC Cmpe_M_A ; Op 1181 - Compare Immediate Memory with Reg E
Cmpe_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	cmp byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpe_M_A ENDP

PUBLIC Cmpu_M_A ; Op 1183 - Compare Immediate Memory with Reg U
Cmpu_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [u_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpu_M_A ENDP

PUBLIC Lde_M_A ; Op 1186 - Load E from Immediate Memory
Lde_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lde_M_A ENDP

PUBLIC Adde_M_A ; Op 118B - Add Immediate Memory with Reg E
Adde_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	add byte ptr [q_s+EREG], al
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
Adde_M_A ENDP

PUBLIC Cmps_M_A ; Op 118C - Compare Immediate Memory with Reg S
Cmps_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	cmp word ptr [s_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmps_M_A ENDP

PUBLIC Divd_M_A ; Op 118D - Divide Reg D by Immediate Memory byte
Divd_M_A PROC
	sub rsp, 28h
  ; Get postbyte
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
  ; if postbyte (divisor) is zero raise interupt
  cmp al, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp16 = DREG / postbyte
  movsx ecx, al ; mov divsor to cx sign extended
  movsx eax, word ptr [q_s+DREG]
  cdq ; sign extend dividend ax into dx:ax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 255 || quotient < -256
  cmp ax, 255
  jg overrange
  cmp ax, 0ff00h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov byte ptr [q_s+AREG], dl ; remainder
  mov byte ptr [q_s+BREG], al ; quotient
  ; check if overflow
  ; if quotient > 127 || quotient < -256
  cmp ax, 127
  jg overflow
  cmp ax, 0ff80h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add al, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divd_M_A ENDP

PUBLIC Divq_M_A ; Op 118E - Divide Reg Q by Immediate Memory word
Divq_M_A PROC
	sub rsp, 28h
  ; Get postword
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
  ; if postword (divisor) is zero raise interupt
  cmp ax, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp32 = QREG / postword
  movsx ecx, ax ; mov divsor to rcx sign extended
  mov eax, dword ptr [q_s]
  cdq ; sign extend dividend rax into rdx:rax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 65535 || quotient < -65536
  cmp eax, 65535
  jg overrange
  cmp eax, 0ffff0000h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov word ptr [q_s+DREG], dx ; remainder
  mov word ptr [q_s+WREG], ax ; quotient
  ; check if overflow
  ; if quotient > 32767 || quotient < -32768
  cmp eax, 32767
  jg overflow
  cmp eax, 0ffff8000h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divq_M_A ENDP

PUBLIC Muld_M_A ; Op 118F - Multiply Reg D by Immediate Memory word
Muld_M_A PROC
  sub rsp, 28h
  ; Get postword
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s ; ax = postword
  movsx eax, ax
  movsx ecx, word ptr [q_s+DREG]
  imul eax, ecx
  mov dword ptr [q_s], eax
  ; set flags
  add eax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+CF_C_B], 0
  mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Muld_M_A ENDP

PUBLIC Sube_D_A ; Op 1190 - Subtract DP Memory from Reg E
Sube_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sube_D_A ENDP

PUBLIC Cmpe_D_A ; Op 1191 - Compare DP Memory with Reg E
Cmpe_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpe_D_A ENDP

PUBLIC Cmpu_D_A ; Op 1193 - Compare DP Memory with Reg U
Cmpu_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem word
	call MemRead16_s
	cmp word ptr [u_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpu_D_A ENDP

PUBLIC Lde_D_A ; Op 1196 - Load E from DP Memory
Lde_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lde_D_A ENDP

PUBLIC Ste_D_A ; Op 1197 - Store E to DP Memory
Ste_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	or dx, ax ; dx is memoery address
	mov cl, byte ptr [q_s+EREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Ste_D_A ENDP

PUBLIC Adde_D_A ; Op 119B - Add DP Memory with Reg E
Adde_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+EREG], al
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
Adde_D_A ENDP

PUBLIC Cmps_D_A ; Op 119C - Compare DP Memory with Reg S
Cmps_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem word
	call MemRead16_s
	cmp word ptr [s_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmps_D_A ENDP

PUBLIC Divd_D_A ; Op 119D - Divide Reg D by DP Memory byte
Divd_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
  ; if postbyte (divisor) is zero raise interupt
  cmp al, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp16 = DREG / postbyte
  movsx ecx, al ; mov divsor to cx sign extended
  movsx eax, word ptr [q_s+DREG]
  cdq ; sign extend dividend ax into dx:ax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 255 || quotient < -256
  cmp ax, 255
  jg overrange
  cmp ax, 0ff00h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov byte ptr [q_s+AREG], dl ; remainder
  mov byte ptr [q_s+BREG], al ; quotient
  ; check if overflow
  ; if quotient > 127 || quotient < -256
  cmp ax, 127
  jg overflow
  cmp ax, 0ff80h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add al, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divd_D_A ENDP

PUBLIC Divq_D_A ; Op 119E - Divide Reg Q by DP Memory word
Divq_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem word
	call MemRead16_s
  ; if postword (divisor) is zero raise interupt
  cmp ax, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp32 = QREG / postword
  movsx ecx, ax ; mov divsor to rcx sign extended
  mov eax, dword ptr [q_s]
  cdq ; sign extend dividend rax into rdx:rax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 65535 || quotient < -65536
  cmp eax, 65535
  jg overrange
  cmp eax, 0ffff0000h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov word ptr [q_s+DREG], dx ; remainder
  mov word ptr [q_s+WREG], ax ; quotient
  ; check if overflow
  ; if quotient > 32767 || quotient < -32768
  cmp eax, 32767
  jg overflow
  cmp eax, 0ffff8000h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divq_D_A ENDP

PUBLIC Muld_D_A ; Op 119F - Multiply Reg D by DP Memory word
Muld_D_A PROC
  sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem word
	call MemRead16_s ; ax = dp word
  movsx eax, ax
  movsx ecx, word ptr [q_s+DREG]
  imul eax, ecx
  mov dword ptr [q_s], eax
  ; set flags
  add eax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+CF_C_B], 0
  mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Muld_D_A ENDP

PUBLIC Sube_X_A ; Op 11A0 - Subtract Indexed Memory from Reg E
Sube_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	sub byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sube_X_A ENDP

PUBLIC Cmpe_X_A ; Op 11A1 - Compare Indexed Memory with Reg E
Cmpe_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	cmp byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpe_X_A ENDP

PUBLIC Cmpu_X_A ; Op 11A3 - Compare Indexed Memory with Reg U
Cmpu_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead16_s
	cmp word ptr [u_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpu_X_A ENDP

PUBLIC Lde_X_A ; Op 11A6 - Load E from Indexed Memory
Lde_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lde_X_A ENDP

PUBLIC Ste_X_A ; Op 11A7 - Store E to Indexed Memory
Ste_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov dx, ax
	mov cl, byte ptr [q_s+EREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Ste_X_A ENDP

PUBLIC Adde_X_A ; Op 11AB - Add Indexed Memory with Reg E
Adde_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	add byte ptr [q_s+EREG], al
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
Adde_X_A ENDP

PUBLIC Cmps_X_A ; Op 11AC - Compare Indexed Memory with Reg S
Cmps_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead16_s
	cmp word ptr [s_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmps_X_A ENDP

PUBLIC Divd_X_A ; Op 11AD - Divide Reg D by Indexed Memory byte
Divd_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
  ; if postbyte (divisor) is zero raise interupt
  cmp al, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp16 = DREG / postbyte
  movsx ecx, al ; mov divsor to cx sign extended
  movsx eax, word ptr [q_s+DREG]
  cdq ; sign extend dividend ax into dx:ax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 255 || quotient < -256
  cmp ax, 255
  jg overrange
  cmp ax, 0ff00h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov byte ptr [q_s+AREG], dl ; remainder
  mov byte ptr [q_s+BREG], al ; quotient
  ; check if overflow
  ; if quotient > 127 || quotient < -256
  cmp ax, 127
  jg overflow
  cmp ax, 0ff80h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add al, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divd_X_A ENDP

PUBLIC Divq_X_A ; Op 11AE - Divide Reg Q by Indexed Memory word
Divq_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead16_s
  ; if postword (divisor) is zero raise interupt
  cmp ax, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp32 = QREG / postword
  movsx ecx, ax ; mov divsor to rcx sign extended
  mov eax, dword ptr [q_s]
  cdq ; sign extend dividend rax into rdx:rax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 65535 || quotient < -65536
  cmp eax, 65535
  jg overrange
  cmp eax, 0ffff0000h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov word ptr [q_s+DREG], dx ; remainder
  mov word ptr [q_s+WREG], ax ; quotient
  ; check if overflow
  ; if quotient > 32767 || quotient < -32768
  cmp eax, 32767
  jg overflow
  cmp eax, 0ffff8000h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divq_X_A ENDP

PUBLIC Muld_X_A ; Op 11AF - Multiply Reg D by Indexed Memory word
Muld_X_A PROC
  sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead16_s ; ax = dp word
  movsx eax, ax
  movsx ecx, word ptr [q_s+DREG]
  imul eax, ecx
  mov dword ptr [q_s], eax
  ; set flags
  add eax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+CF_C_B], 0
  mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Muld_X_A ENDP

PUBLIC Sube_E_A ; Op 11B0 - Subtract Extended Memory from Reg E
Sube_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	sub byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Sube_E_A ENDP

PUBLIC Cmpe_E_A ; Op 11B1 - Compare Extended Memory with Reg E
Cmpe_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	cmp byte ptr [q_s+EREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpe_E_A ENDP

PUBLIC Cmpu_E_A ; Op 11B3 - Compare Extended Memory with Reg U
Cmpu_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [u_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpu_E_A ENDP

PUBLIC Lde_E_A ; Op 11B6 - Load E from Extended Memory
Lde_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	mov byte ptr [q_s+EREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Lde_E_A ENDP

PUBLIC Ste_E_A ; Op 11B7 - Store E to Extended Memory
Ste_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
  mov dx, ax
	mov cl, byte ptr [q_s+EREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Ste_E_A ENDP

PUBLIC Adde_E_A ; Op 11BB - Add Indexed Extended with Reg E
Adde_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	add byte ptr [q_s+EREG], al
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
Adde_E_A ENDP

PUBLIC Cmps_E_A ; Op 11BC - Compare Indexed Extended with Reg S
Cmps_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
	cmp word ptr [s_s], ax
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmps_E_A ENDP

PUBLIC Divd_E_A ; Op 11BD - Divide Reg D by Extended Memory byte
Divd_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
  ; if postbyte (divisor) is zero raise interupt
  cmp al, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp16 = DREG / postbyte
  movsx ecx, al ; mov divsor to cx sign extended
  movsx eax, word ptr [q_s+DREG]
  cdq ; sign extend dividend ax into dx:ax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 255 || quotient < -256
  cmp ax, 255
  jg overrange
  cmp ax, 0ff00h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov byte ptr [q_s+AREG], dl ; remainder
  mov byte ptr [q_s+BREG], al ; quotient
  ; check if overflow
  ; if quotient > 127 || quotient < -256
  cmp ax, 127
  jg overflow
  cmp ax, 0ff80h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add al, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divd_E_A ENDP

PUBLIC Divq_E_A ; Op 11BE - Divide Reg Q by Extended Memory word
Divq_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s
  ; if postword (divisor) is zero raise interupt
  cmp ax, 0
  jne notdivbyzero
  call DivbyZero_s
  jmp exit
notdivbyzero:
  ; stemp32 = QREG / postword
  movsx ecx, ax ; mov divsor to rcx sign extended
  mov eax, dword ptr [q_s]
  cdq ; sign extend dividend rax into rdx:rax
  idiv ecx ; edx:eax / ecx (eax quotient - edx remainder)
  ; check if over range
  ; if quotient > 65535 || quotient < -65536
  cmp eax, 65535
  jg overrange
  cmp eax, 0ffff0000h
  jge notoverrange
overrange:
  ; then over range set flags and exit
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 0
  mov byte ptr [cc_s+ZF_C_B], 0
  mov byte ptr [cc_s+CF_C_B], 0
  jmp exit
notoverrange:
  mov word ptr [q_s+DREG], dx ; remainder
  mov word ptr [q_s+WREG], ax ; quotient
  ; check if overflow
  ; if quotient > 32767 || quotient < -32768
  cmp eax, 32767
  jg overflow
  cmp eax, 0ffff8000h
  jge notoverflow
overflow:
  ; then is overflow set flags
  mov byte ptr [cc_s+VF_C_B], 1
  mov byte ptr [cc_s+NF_C_B], 1
  jmp setcarry
notoverflow:
  ; set flags
  add ax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
setcarry:
  ; always determine carry
  bt ax, 0 ; if the lower order bit is 1 (odd)
	setc byte ptr [cc_s+CF_C_B]
exit:
	add rsp, 28h
	ret
Divq_E_A ENDP

PUBLIC Muld_E_A ; Op 11BF - Multiply Reg D by Extended Memory word
Muld_E_A PROC
  sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead16_s ; ax = dp word
  movsx eax, ax
  movsx ecx, word ptr [q_s+DREG]
  imul eax, ecx
  mov dword ptr [q_s], eax
  ; set flags
  add eax, 0
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+CF_C_B], 0
  mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Muld_E_A ENDP

PUBLIC Subf_M_A ; Op 11C0 - Subtract Immediate Memory from Reg F
Subf_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	sub byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subf_M_A ENDP

PUBLIC Cmpf_M_A ; Op 11C1 - Compare Immediate Memory with Reg F
Cmpf_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	cmp byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpf_M_A ENDP

PUBLIC Ldf_M_A ; Op 11C6 - Load F from Immediate Memory
Ldf_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldf_M_A ENDP

PUBLIC Addf_M_A ; Op 11CB - Add Immediate Memory with Reg F
Addf_M_A PROC
	sub rsp, 28h
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	add byte ptr [q_s+FREG], al
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
Addf_M_A ENDP

PUBLIC Subf_D_A ; Op 11D0 - Subtract DP Memory from Reg F
Subf_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	sub byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subf_D_A ENDP

PUBLIC Cmpf_D_A ; Op 11D1 - Compare DP Memory with Reg F
Cmpf_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	cmp byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpf_D_A ENDP

PUBLIC Ldf_D_A ; Op 11D6 - Load F from DP Memory
Ldf_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldf_D_A ENDP

PUBLIC Stf_D_A ; Op 11D7 - Store F to DP Memory
Stf_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov dx, word ptr [dp_s]
	or dx, ax ; dx is memoery address
	mov cl, byte ptr [q_s+FREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Stf_D_A ENDP

PUBLIC Addf_D_A ; Op 11DB - Add DP Memory with Reg F
Addf_D_A PROC
	sub rsp, 28h
	; Calc Address by Adding the Byte at [PC++] with DP reg
	mov cx, word ptr [pc_s]
	inc word ptr [pc_s]
	call MemRead8_s
	mov cx, word ptr [dp_s]
	or cx, ax
	; and use it to get a mem byte
	call MemRead8_s
	add byte ptr [q_s+FREG], al
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
Addf_D_A ENDP

PUBLIC Subf_X_A ; Op 11E0 - Subtract Indexed Memory from Reg F
Subf_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	sub byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subf_X_A ENDP

PUBLIC Cmpf_X_A ; Op 11E1 - Compare Indexed Memory with Reg F
Cmpf_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	cmp byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpf_X_A ENDP

PUBLIC Ldf_X_A ; Op 11E6 - Load F from Indexed Memory
Ldf_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldf_X_A ENDP

PUBLIC Stf_X_A ; Op 11E7 - Store F to Indexed Memory
Stf_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov dx, ax
	mov cl, byte ptr [q_s+FREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Stf_X_A ENDP

PUBLIC Addf_X_A ; Op 11EB - Add Indexed Memory with Reg F
Addf_X_A PROC
	sub rsp, 28h
	call CalcEA_A
  mov cx, ax
	call MemRead8_s
	add byte ptr [q_s+FREG], al
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
Addf_X_A ENDP

PUBLIC Subf_E_A ; Op 11F0 - Subtract Extended Memory from Reg F
Subf_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	sub byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Subf_E_A ENDP

PUBLIC Cmpf_E_A ; Op 11F1 - Compare Extended Memory with Reg F
Cmpf_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	cmp byte ptr [q_s+FREG], al
	; Save flags NF+ZF+VF+CF
	setc byte ptr [cc_s+CF_C_B]
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	seto byte ptr [cc_s+VF_C_B]
	add rsp, 28h
	ret
Cmpf_E_A ENDP

PUBLIC Ldf_E_A ; Op 11F6 - Load F from Extended Memory
Ldf_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	mov byte ptr [q_s+FREG], al
	; trigger and save flags
	add al, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	add rsp, 28h
	ret
Ldf_E_A ENDP

PUBLIC Stf_E_A ; Op 11F7 - Store F to Extended Memory
Stf_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
  mov dx, ax
	mov cl, byte ptr [q_s+FREG]
	; trigger and save flags
	add cl, 0
	; Save flags NF+ZF+VF
	setz byte ptr [cc_s+ZF_C_B]
	sets byte ptr [cc_s+NF_C_B]
	mov byte ptr [cc_s+VF_C_B], 0
	; use it to get a mem word
	call MemWrite8_s ; cx - data, dx address
	add rsp, 28h
	ret
Stf_E_A ENDP

PUBLIC Addf_E_A ; Op 11FB - Add Indexed Extended with Reg F
Addf_E_A PROC
	sub rsp, 28h
	; Get immediate two bytes at PC
	mov cx, word ptr [pc_s]
	add word ptr [pc_s], 2
	call MemRead16_s
	mov cx, ax
	; Use address to get a mem word
	call MemRead8_s
	add byte ptr [q_s+FREG], al
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
Addf_E_A ENDP
END