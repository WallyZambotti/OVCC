INCLUDE hd6309asm_h.asm

EXTERN	getcc_s: PROC
EXTERN	setcc_s: PROC

.data

.code

PUBLIC Addr_A ; Op 1030 - Add register to register
Addr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  add al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; add the 16 bit registers
  add ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Addr_A ENDP

PUBLIC Adcr_A ; Op 1031 - Add register to register with carry
Adcr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers with carry
doop:
  bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
  adc al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; add the 16 bit registers with carry
  bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
  adc ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Adcr_A ENDP

PUBLIC Subr_A ; Op 1032 - Sub register from register
Subr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  sub al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; add the 16 bit registers
  sub ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Subr_A ENDP

PUBLIC Sbcr_A ; Op 1033 - Sub register from register with borrow
Sbcr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; sub the 8 bit registers with borrow
doop:
  bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
  sbb al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; sub the 16 bit registers with borrow
  bt word ptr [cc_s+CF_C_B], 0 ; bit 0 (1st) is CF
  sbb ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Sbcr_A ENDP

PUBLIC Andr_A ; Op 1034 - And register with register
Andr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  and al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; and the 16 bit registers
  and ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Andr_A ENDP

PUBLIC Orr_A ; Op 1035 - Or register with register
Orr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  or al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; or the 16 bit registers
  or ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Orr_A ENDP

PUBLIC Eorr_A ; Op 1036 - Eor register with register
Eorr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  xor al, dl
  ; if the 8 bit reg is CC then call setcc_s otherwise save to reg
  lea r8, switch1
  mov r8, [r8+r9*8]
  jmp r8
switch1:
  QWORD savenotcc
  QWORD savenotcc
  QWORD savecc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
  QWORD savenotcc
savecc:
  ; case cc reg
  mov cl, al
  setc al ; save CF
  call setcc_s
  bt ax, 0 ; restore CF
  jmp saveflags
savenotcc:
  ; case otherwise
  mov byte ptr [rcx], al ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
saveflags:
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; Eor the 16 bit registers
  xor ax, dx
  mov word ptr [rcx], ax ; save the result to destination reg
  ; Save flags NF+ZF+VF+CF
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  mov byte ptr [cc_s+VF_C_B], 0
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Eorr_A ENDP

PUBLIC Cmpr_A ; Op 1037 - Cmp register with register
Cmpr_A PROC
  sub rsp, 28h
  ; Get Post Byte at PC++
  mov cx, word ptr [pc_s]
  inc word ptr [pc_s]
  call MemRead8_s
  mov dl, al ; copy postbyte
  and ax, 0Fh ; low nible destination register
  shr dl, 4 ; high nible source register
  cmp al, 7 ; if dest reg is > 7 then dest is 8 bit
  jle dest16 ; otherwise dest is 16 bit
  ; fetch the source reg
  cmp dl, 7 ; if source reg > 7 then bit source is 8 bit
  jle source16 ; otherwise source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dl(source) = *ureg8_s[(dl)source_reg_number] (except cc reg 2)
  ; if cc reg
  cmp dl, 2
  jne notcc
  call getcc_s
  mov dl, al
  jmp fetchdest
  ; else is not cc reg
notcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dl, byte ptr [rcx]
  jmp fetchdest
source16:
  ; else source is 16 bit
  and rdx, 7 ; strip reg size bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
fetchdest:
  ; fetch the destination reg
  and rax, 7 ; strip reg size bit
  mov r9, rax
  ; if the destination register is CC then have to getcc_s explicitly
  cmp al, 2 ; is it the CC reg
  jne getdestnotcc
  call getcc_s
  jmp doop
  ; else is not cc reg
  ; al(dest) = *ureg8_s[(al)dest_reg_number]
getdestnotcc:
  lea rcx, word ptr [ureg8_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov al, byte ptr [rcx]
  ; add the 8 bit registers
doop:
  sub al, dl
  ; Save flags NF+ZF+VF+CF
saveflags:
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  jmp exit
  ; else Destination is 16 bit
dest16:
  ; if source reg < 8 
  cmp dl, 8 
  jge source8
  ; then source is 16 bit
  ; dx(source) = *xfreg16_s[(dl)source_reg_number]
  and rdx, 7 ; strip reg size bit
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rdx*8]
  mov dx, word ptr [rcx]
  jmp fetchdest_2
  ; else source 8 bit promote to 16 bit
source8:
  ; switch (dl reg_num)
  and rdx, 7 ; strip reg size bit
  lea rcx, switch2
  mov rcx, [rcx+rdx*8]
  jmp rcx
switch2:
  QWORD case00
  QWORD case01
  QWORD case02
  QWORD case03
  QWORD case04
  QWORD case05
  QWORD case06
  QWORD case07
case00: ; Case Source Reg is A or B promote to D
case01:
  mov dx, word ptr [q_s+DREG]
  jmp fetchdest_2
case02: ; Case Source is CC
  mov dx, ax ; getcc_s destroys ax so save it
  call getcc_s
  xchg dx, ax ; put result in dx and restore ax
  jmp fetchdest_2
case03: ; Case Source is DP
  mov dx, word ptr [dp_s]
  jmp fetchdest_2
case04: ; Case Source is Z
case05:
  mov dx, 0
  jmp fetchdest_2
case06: ; Case Source is E or F promote to W
case07:
  mov dx, word ptr [q_s+WREG]
fetchdest_2: 
  ; load the destination reg
  ; ax(source) = *xfreg16_s[(al)source_reg_number]
  lea rcx, word ptr [xfreg16_s]
  mov rcx, qword ptr [rcx+rax*8]
  mov ax, word ptr [rcx]
  ; add the 16 bit registers
  sub ax, dx
  ; Save flags NF+ZF+VF+CF
  setc byte ptr [cc_s+CF_C_B]
  setz byte ptr [cc_s+ZF_C_B]
  sets byte ptr [cc_s+NF_C_B]
  seto byte ptr [cc_s+VF_C_B]
  mov word ptr [z_s], 0 ; incase the dest reg was the zero reg
exit:
  add rsp, 28h
  ret
Cmpr_A ENDP
END