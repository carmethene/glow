//normal.vsh - transforms all vertices by a supplied matrix
//and outputs a lit diffuse vertex colour

//c0..3	transposed world-view-projection matrix
//c4	material colour
//c5	light vector
//c6	ambient light

vs.1.1

dcl_position	v0
dcl_normal		v1

def	c5, 0.5, 0.5, -0.5, 0.0
def c6, 0.4, 0.0, 0.0, 0.0

//transformed vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//diffuse light coefficient
dp3 r0.x, v1, c5
max r0.x, r0.x, c5.a

//output vertex colour
mul r0, r0.xxxx, c4
mov r1, c6.xxxx
mad r1, r1, c4, r0

//zero alpha as we do not want glow
mov r1.a, c5.a
mov oD0, r1
