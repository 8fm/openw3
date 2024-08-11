#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.20499
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// tex                               sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
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
//
// Sampler/Resource to DX9 shader sampler mappings:
//
// Target Sampler Source Sampler  Source Resource
// -------------- --------------- ----------------
// s0             s0              t0               
//
//
// Level9 shader bytecode:
//
    ps_2_x
    def c0, -0.501960993, -0, 1.56864798, 1.84835196
    def c1, 0.186592996, 0.466295987, 1, 0
    dcl t0
    dcl t1
    dcl_pp t2.xy
    dcl_2d s0
    texld r0, t2, s0
    add r0.xyz, r0, c0.xyxw
    mad r0.w, r0.x, -c1.x, r0.y
    mad_sat r1.y, r0.z, -c1.y, r0.w
    mad_sat r1.xz, r0.zyxw, c0.zyww, r0.y
    mov r1.w, c1.z
    mov r0, t1
    mad r0, r1, r0, t0
    mov oC0, r0

// approximately 9 instruction slots used (1 texture, 8 arithmetic)
ps_4_0
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v2.xyxx, t0.xyzw, s0
add r0.xyz, r0.xyzx, l(-0.501961, -0.000000, -0.501961, 0.000000)
mad r0.w, -r0.x, l(0.186593), r0.y
mad_sat r1.y, -r0.z, l(0.466296), r0.w
mad_sat r1.xz, r0.zzxz, l(1.568648, 0.000000, 1.848352, 0.000000), r0.yyyy
mov r1.w, l(1.000000)
mad o0.xyzw, r1.xyzw, v1.xyzw, v0.xyzw
ret 
// Approximately 8 instruction slots used
#endif

const BYTE pBinary_D3D1xFL93_FYUY2Cxform[] =
{
     68,  88,  66,  67, 100, 237, 
    195, 234, 133, 149, 133, 151, 
     32, 185, 118,  56, 111,  77, 
    245, 122,   1,   0,   0,   0, 
    128,   4,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    100,   1,   0,   0, 208,   2, 
      0,   0,  76,   3,   0,   0, 
    232,   3,   0,   0,  76,   4, 
      0,   0,  65, 111, 110,  57, 
     36,   1,   0,   0,  36,   1, 
      0,   0,   0,   2, 255, 255, 
    252,   0,   0,   0,  40,   0, 
      0,   0,   0,   0,  40,   0, 
      0,   0,  40,   0,   0,   0, 
     40,   0,   1,   0,  36,   0, 
      0,   0,  40,   0,   0,   0, 
      0,   0,   1,   2, 255, 255, 
     81,   0,   0,   5,   0,   0, 
     15, 160, 132, 128,   0, 191, 
      0,   0,   0, 128, 117, 201, 
    200,  63, 204, 150, 236,  63, 
     81,   0,   0,   5,   1,   0, 
     15, 160,  60,  18,  63,  62, 
     89, 190, 238,  62,   0,   0, 
    128,  63,   0,   0,   0,   0, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   1,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   2,   0,  35, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     66,   0,   0,   3,   0,   0, 
     15, 128,   2,   0, 228, 176, 
      0,   8, 228, 160,   2,   0, 
      0,   3,   0,   0,   7, 128, 
      0,   0, 228, 128,   0,   0, 
    196, 160,   4,   0,   0,   4, 
      0,   0,   8, 128,   0,   0, 
      0, 128,   1,   0,   0, 161, 
      0,   0,  85, 128,   4,   0, 
      0,   4,   1,   0,  18, 128, 
      0,   0, 170, 128,   1,   0, 
     85, 161,   0,   0, 255, 128, 
      4,   0,   0,   4,   1,   0, 
     21, 128,   0,   0, 198, 128, 
      0,   0, 246, 160,   0,   0, 
     85, 128,   1,   0,   0,   2, 
      1,   0,   8, 128,   1,   0, 
    170, 160,   1,   0,   0,   2, 
      0,   0,  15, 128,   1,   0, 
    228, 176,   4,   0,   0,   4, 
      0,   0,  15, 128,   1,   0, 
    228, 128,   0,   0, 228, 128, 
      0,   0, 228, 176,   1,   0, 
      0,   2,   0,   8,  15, 128, 
      0,   0, 228, 128, 255, 255, 
      0,   0,  83,  72,  68,  82, 
    100,   1,   0,   0,  64,   0, 
      0,   0,  89,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   2,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,  10, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    132, 128,   0, 191,   0,   0, 
      0, 128, 132, 128,   0, 191, 
      0,   0,   0,   0,  50,   0, 
      0,  10, 130,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,  60,  18,  63,  62, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  50,  32,   0,  10, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
     89, 190, 238,  62,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     50,  32,   0,  12,  82,   0, 
     16,   0,   1,   0,   0,   0, 
    166,   8,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    117, 201, 200,  63,   0,   0, 
      0,   0, 204, 150, 236,  63, 
      0,   0,   0,   0,  86,   5, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,   9, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,   8,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  82,  68, 
     69,  70, 148,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0, 145,   0,   0, 
     96,   0,   0,   0,  92,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     92,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  12,   0, 
      0,   0, 116, 101, 120,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  57, 
     46,  51,  48,  46,  57,  50, 
     48,  48,  46,  50,  48,  52, 
     57,  57,   0, 171,  73,  83, 
     71,  78,  92,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   3,   3, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171
};
