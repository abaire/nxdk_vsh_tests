; Values from c[188], c[189], c[190], c[191] will be captured
#input matrix4 96
#output matrix4 188

mov r0, #input[0]
mov r1, #input[1]
mov r2, #input[2]
mad #output[0], r0, r1, r2
