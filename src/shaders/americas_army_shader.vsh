; Values from c[188], c[189], c[190], c[191] will be captured
#input matrix4 96
#output matrix4 188

; Inputs:
; c[121] = (2.0, 2.0, 2.0, 1.0)
; c[135] = (0.0, 0.5, 1.0, 3.0)
; c[137] = (-0.0567268, -0.178603, -0.9822846, 0.0)
; c[138] = (0.0, 0.9838689, -0.1788911, 0.0)
; c[139] = (31.5401363, -20.6333656, 67.6486282, 1.0)
; c[140] = (0.0, 0.0, 0.0, 0.0)
; c[141] = (0.0, 0.0, 0.0, 0.0)
; c[142] = (0.0, 0.0, 0.0, 0.0)
; c[143] = (0.0, 0.0, 0.0, 0.0)
; c[144] = (0.0, 0.0, 0.0, 0.0)
; c[145] = (1.0, 0.0, 0.0, 0.0)
; c[146] = (0.0, 1.0, 0.0, 0.0)
; c[147] = (0.0, 0.0, 1.0, 0.0)
; c[98] = (-0.055816, -0.9823595, -0.1789047, 62.6534042)
; v0 (from c[96]) = (0.1647059, 0.1647059, 0.1686275, 1.0)
; v1 (from c[97]) = (-728.0, -4058.0, 0.0, 0.0)

;MUL R6.xyzw, v1, c[121]
mov r3, #input[1]
MUL R6.xyzw, r3, c[121]

ADD R10.yzw, R6.yzx, -c[137].yzx
MUL R2.yzw, R10.yzw, R10.yzw
ADD R11.xyz, R6.xyz, -c[138].xyz
DP4 oPos.x, R6, c[96] + RSQ R1.x, R2.x
DP3 R2.y, c[135].z, R2.wyz
MUL R11.yzw, R11.yzx, R11.yzx
ADD R10.xyz, R6.xyz, -c[139].xyz
DST R9.xyz, R2.x, R1.x
DP3 R2.x, c[135].z, R11.wyz + RSQ R1.x, R2.y
MUL R11.yzw, R10.yzx, R10.yzx
DST R10.xyz, R2.y, R1.x
DP3 R2.y, c[135].z, R11.wyz + RSQ R1.x, R2.x
DST R8.xyz, R2.x, R1.x
DP3 R11.y, R10.xyz, c[145].xyz + RSQ R1.x, R2.y
SLT R0.w, R9.y, c[144].w + RCP R1.y, R11.x
DST R11.xyz, R2.y, R1.x + RCP R1.z, R11.y
MUL R7.xyz, R1.y, c[140].xyz
DP3 R11.w, R8.xyz, c[146].xyz
MUL R7.xyz, R7.xyz, R0.w
MUL R10.xzw, R1.z, c[141].yzx + RCP R1.x, R11.w
SLT R0.w, R10.y, c[145].w
DP3 R11.x, R11.xyz, c[147].xyz

; ADD R8.xzw, v0.xzy, R7.xzy
mov r3, #input[0]
ADD R8.xzw, r3.xzy, R7.xzy

MUL R7.xyz, R10.wxz, R0.w + RCP R1.y, R11.x
MUL R11.xzw, R1.x, c[142].yzx
SLT R0.w, R8.y, c[146].w
ADD R8.xyz, R8.xwz, R7.xyz
MUL R7.xyz, R11.wxz, R0.w
MUL R11.xzw, R1.y, c[143].yzx
DP4 oPos.z, R6, c[98] + DP4 R0.x, R6, c[98]
ADD R8.xyz, R8.xyz, R7.xyz
MUL R7.xyz, R11.wxz, R0.w + RCC R1.x, R12.w
ADD oD0.xyz, R8.xyz, R7.xyz

add #output[0].xyz, R8.xyz, R7.xyz
