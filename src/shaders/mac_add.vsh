; Values from c[188], c[189], c[190], c[191] will be captured
#input matrix4 96
#output matrix4 188

; Only one constant may be referenced in a command.
mov r0, #input[0]
mov r1, #input[1]
add #output, r0, r1
