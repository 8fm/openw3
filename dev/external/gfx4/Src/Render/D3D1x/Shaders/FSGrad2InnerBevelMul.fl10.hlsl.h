#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.20499
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
// {
//
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 srctexscale;                // Offset:   32 Size:    16
//   float4 texscale;                   // Offset:   48 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_gradtex                   sampler      NA          NA    0        1
// sampler_srctex                    sampler      NA          NA    1        1
// sampler_tex                       sampler      NA          NA    2        1
// gradtex                           texture  float4          2d    0        1
// srctex                            texture  float4          2d    1        1
// tex                               texture  float4          2d    2        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_4_0
dcl_constantbuffer cb0[4], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 4
mov r0.xy, l(0,0,0,0)
mov r0.z, -cb0[0].x
loop 
  lt r0.w, cb0[0].x, r0.z
  breakc_nz r0.w
  add r1.x, r0.z, cb0[1].x
  mov r2.xy, r0.xyxx
  mov r2.z, -cb0[0].y
  loop 
    lt r0.w, cb0[0].y, r2.z
    breakc_nz r0.w
    add r1.y, r2.z, cb0[1].y
    mad r1.zw, r1.xxxy, cb0[3].xxxy, v2.xxxy
    sample_l r3.xyzw, r1.zwzz, t2.xyzw, s2, l(0.000000)
    add r2.y, r2.y, r3.w
    mad r1.yz, -r1.xxyx, cb0[3].xxyx, v2.xxyx
    sample_l r3.xyzw, r1.yzyy, t2.xyzw, s2, l(0.000000)
    add r2.x, r2.x, r3.w
    add r2.z, r2.z, l(1.000000)
  endloop 
  mov r0.xy, r2.xyxx
  add r0.z, r0.z, l(1.000000)
endloop 
mad r0.xy, -r0.xyxx, cb0[0].wwww, l(1.000000, 1.000000, 0.000000, 0.000000)
add r0.xy, -r0.yxyy, r0.xyxx
mul r0.y, r0.y, cb0[0].z
mad r0.x, -r0.x, cb0[0].z, l(1.000000)
mul r0.x, r0.x, l(0.500000)
max r0.x, r0.x, l(0.000000)
min r1.x, r0.x, l(0.500000)
mov r1.yw, l(0,0,0,0)
sample r2.xyzw, r1.xyxx, t0.xyzw, s0
mad r0.x, r0.y, l(0.500000), l(0.500000)
max r0.x, r0.x, l(0.500000)
min r1.z, r0.x, l(1.000000)
sample r0.xyzw, r1.zwzz, t0.xyzw, s0
mul r1.xy, v2.xyxx, cb0[2].xyxx
sample_l r1.xyzw, r1.xyxx, t1.xyzw, s1, l(0.000000)
mov_sat r3.x, r2.w
mov_sat r3.y, r0.w
mov r2.w, l(1.000000)
mov r0.w, l(1.000000)
mul r0.xyzw, r3.yyyy, r0.xyzw
mad r0.xyzw, r2.xyzw, r3.xxxx, r0.xyzw
add r2.x, -r3.x, l(1.000000)
add r2.x, -r3.y, r2.x
mad r0.xyzw, r1.xyzw, r2.xxxx, r0.xyzw
mul r0.xyzw, r1.wwww, r0.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad r0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
// Approximately 56 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_FSGrad2InnerBevelMul[] =
{
     68,  88,  66,  67, 215, 222, 
    119, 182,  87, 117,  87, 133, 
    225, 221, 248, 148, 106,  51, 
    164, 163,   1,   0,   0,   0, 
     52,  10,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     96,   2,   0,   0, 196,   2, 
      0,   0, 248,   2,   0,   0, 
    184,   9,   0,   0,  82,  68, 
     69,  70,  36,   2,   0,   0, 
      1,   0,   0,   0,  68,   1, 
      0,   0,   7,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0, 129,   0,   0, 
    240,   1,   0,   0, 252,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     12,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  27,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  39,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
     47,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,  54,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   2,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0,  58,   1, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 103, 114,  97, 100, 
    116, 101, 120,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    115, 114,  99, 116, 101, 120, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
      0, 103, 114,  97, 100, 116, 
    101, 120,   0, 115, 114,  99, 
    116, 101, 120,   0, 116, 101, 
    120,   0,  67, 111, 110, 115, 
    116,  97, 110, 116, 115,   0, 
     58,   1,   0,   0,   4,   0, 
      0,   0,  92,   1,   0,   0, 
     64,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    188,   1,   0,   0,   0,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 196,   1, 
      0,   0,   0,   0,   0,   0, 
    212,   1,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 196,   1, 
      0,   0,   0,   0,   0,   0, 
    219,   1,   0,   0,  32,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 196,   1, 
      0,   0,   0,   0,   0,   0, 
    231,   1,   0,   0,  48,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 196,   1, 
      0,   0,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    171, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    111, 102, 102, 115, 101, 116, 
      0, 115, 114,  99, 116, 101, 
    120, 115,  99,  97, 108, 101, 
      0, 116, 101, 120, 115,  99, 
     97, 108, 101,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  57,  46,  51, 
     48,  46,  57,  50,  48,  48, 
     46,  50,  48,  52,  57,  57, 
      0, 171,  73,  83,  71,  78, 
     92,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,   3,   3,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     68,  82, 184,   6,   0,   0, 
     64,   0,   0,   0, 174,   1, 
      0,   0,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   4,   0, 
      0,   0,  54,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   3,   0,   4,   3, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5,  50,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  48,   0, 
      0,   1,  49,   0,   0,   8, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,   3,   0, 
      4,   3,  58,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,  42,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     50,   0,   0,  10, 194,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   4,  16,   0,   1,   0, 
      0,   0,   6, 132,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,  72,   0, 
      0,  11, 242,   0,  16,   0, 
      3,   0,   0,   0, 230,  10, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     26,   0,  16,   0,   2,   0, 
      0,   0,  58,   0,  16,   0, 
      3,   0,   0,   0,  50,   0, 
      0,  11,  98,   0,  16,   0, 
      1,   0,   0,   0,   6,   1, 
     16, 128,  65,   0,   0,   0, 
      1,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   6,  17, 
     16,   0,   2,   0,   0,   0, 
     72,   0,   0,  11, 242,   0, 
     16,   0,   3,   0,   0,   0, 
    150,   5,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      2,   0,   0,   0,   0,  96, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,  58,   0, 
     16,   0,   3,   0,   0,   0, 
      0,   0,   0,   7,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     42,   0,  16,   0,   2,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  22,   0, 
      0,   1,  54,   0,   0,   5, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     50,   0,   0,  14,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
    246, 143,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
    128,  63,   0,   0, 128,  63, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  22,   5,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   8,  34,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     42, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     50,   0,   0,  11,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     42, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  56,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     52,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  51,   0, 
      0,   7,  18,   0,  16,   0, 
      1,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,  54,   0,   0,   8, 
    162,   0,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  69,   0, 
      0,   9, 242,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,   1,  64,   0,   0, 
      0,   0,   0,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,  51,   0,   0,   7, 
     66,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
    230,  10,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   8,  50,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   2,   0, 
      0,   0,  70, 128,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  72,   0,   0,  11, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,   0,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  54,  32, 
      0,   5,  18,   0,  16,   0, 
      3,   0,   0,   0,  58,   0, 
     16,   0,   2,   0,   0,   0, 
     54,  32,   0,   5,  34,   0, 
     16,   0,   3,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   2,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     86,   5,  16,   0,   3,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   2,   0,   0,   0, 
      6,   0,  16,   0,   3,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  18,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      3,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     26,   0,  16, 128,  65,   0, 
      0,   0,   3,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,   6,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   1,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,  32, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
     56,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  23,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  24,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
