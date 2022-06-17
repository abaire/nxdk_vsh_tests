; Values from c[188], c[189], c[190], c[191] will be captured
#output matrix4 188

; Inputs:
; v0: 89, 5156, -151, 417
; c[58] 320.00, -240.00, 16777215.00, 0.00
; c[59] 320.03125, 240.03125, 0.00, 0.00
; c[96] 1.00, 0.00, 0.00, 0.00
; c[97] 0.00, 1.00, 0.00, 0.00
; c[98] 0.00, 0.00, 1.00, 0.00
; c[99] -16.8876857758, -18.2681617737, 3.6487216949, 1.00
; c[120] 1.00, 0.0000610352, 0.0009765625, 1.00
; c[125] 1.00, 1.00, 1.00, 0.00


; v0 is used by the test infrastructure so values are placed in v1 instead.
;MUL R0.yzw, v0.wxyz, c[120].zx
MUL R0.yzw, v1.wxyz, c[120].zx
MOV R0.x, c[125].z

mov #output[3], R0

DP4 R11.z, R0.yzwx, c[98]
DP4 R11.x, R0.yzwx, c[96]
DP4 R11.y, R0.yzwx, c[97]

mov #output[2], R11

DP4 oPos.w, R0.yzwx, c[99]
MOV oPos.xyz, R11
RCC R1.x, R12.w

mov #output[1], R1.xxxx

MUL oPos.xyz, R12.xyz, c[58].xyz
MAD oPos.xyz, R12.xyz, R1.x, c[59].xyz

mov #output[0], R12
