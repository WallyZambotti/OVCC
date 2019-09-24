INCLUDE hd6309asm_h.asm

.data

.code

CF_C_B EQU 00h
VF_C_B EQU 01h
ZF_C_B EQU 02h
NF_C_B EQU 03h
IF_C_B EQU 04h
HF_C_B EQU 05h
FF_C_B EQU 06h
EF_C_B EQU 07h

PUBLIC setcc_s
setcc_s PROC ; cx contains cc
	bt cx, CF_C_B
	setc byte ptr [cc_s+CF_C_B]
	bt cx, VF_C_B
	setc byte ptr [cc_s+VF_C_B]
	bt cx, ZF_C_B
	setc byte ptr [cc_s+ZF_C_B]
	bt cx, NF_C_B
	setc byte ptr [cc_s+NF_C_B]
	bt cx, IF_C_B
	setc byte ptr [cc_s+IF_C_B]
	bt cx, HF_C_B
	setc byte ptr [cc_s+HF_C_B]
	bt cx, FF_C_B
	setc byte ptr [cc_s+FF_C_B]
	bt cx, EF_C_B
	setc byte ptr [cc_s+EF_C_B]
	ret
setcc_s ENDP

PUBLIC getcc_s
getcc_s PROC ; ax returns cc
	mov ax, 0
	bt word ptr [cc_s+EF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+FF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+HF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+IF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+NF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+ZF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+VF_C_B], 0
	rcl ax, 1
	bt word ptr [cc_s+CF_C_B], 0
	rcl ax, 1
	ret
getcc_s ENDP
END