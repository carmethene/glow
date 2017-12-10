//glow.vsh - transforms all vertices by a supplied matrix
//and outputs an unlit diffuse colour with high emissivity

//c0..3	transposed world-view-projection matrix
//c4	material colour
//c5	emissivity

vs.1.1

dcl_position	v0
dcl_normal		v1

def c5, 0.8, 0.0, 0.0, 0.0

//transformed vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//unlit vertex colour
mov r0, c4
mov r0.a, c5.r
mov oD0, r0
