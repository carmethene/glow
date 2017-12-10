//gaussianx.vsh - renders a screen-aligned quad and outputs texture
//coords for 7 reads along the x direction
//c0.x	step distance in texture space
//c0.y	half step distance

vs.1.1

dcl_position	v0
dcl_texcoord0	v1
def c1, 1,1,1,1

//output vertex position
mov oPos, v0

//output texture coordinates...
//center
mov r1, v1	//offset to compensate for blur being moved by linear filtering
add r1.x, r1.x, c0.y
mov oT0, r1

//+ve
mov r0, r1
add r0.x, r0.x, c0.x
mov oT1, r0
add r0.x, r0.x, c0.x
mov oT2, r0
add r0.x, r0.x, c0.x
mov oT3, r0

//-ve
mov r0, r1
sub r0.x, r0.x, c0.x
mov oT4, r0
sub r0.x, r0.x, c0.x
mov oT5, r0
sub r0.x, r0.x, c0.x
mov oT6, r0

mov oD0, c1
