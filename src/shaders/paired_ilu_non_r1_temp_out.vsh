; Values from c[188], c[189], c[190], c[191] will be captured
#input matrix4 96
#output matrix4 188

// Control: Set R1.x to 1/(sqrt(c97.x)
MOV R6, #input[0]

DP4 oD0.x, R6, #input[0]
+ RSQ R1.x, #input[0].x

mov #output[0], R1

// Reset R1 to a known value other than the result from the last op.
mov R1, #input[0]
mov R10, #input[0]
mov #output[1], R1

// Do the same calculation with a non-R1 register and verify that it actually
// writes to R1.
DP4 oD0.x, R6, #input[0]
+ RSQ R10.x, #input[0].x

mov #output[2], R1
mov #output[3], R10
