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
// tex                               sampler      NA          NA    0        3
// tex                               texture  float4          2d    0        3
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
// s1             s1              t1               
// s2             s2              t2               
//
//
// Level9 shader bytecode:
//
    ps_2_0
    def c0, -0.0627451017, -0.501960814, 1.16400003, 1
    def c1, 1.59599996, -0.813000023, 0, 0
    def c2, 0, -0.39199999, 2.01699996, 0
    dcl t0
    dcl t1
    dcl t2.xy
    dcl_2d s0
    dcl_2d s1
    dcl_2d s2
    texld r0, t2, s2
    texld r1, t2, s0
    texld r2, t2, s1
    add r0.x, r0.x, c0.y
    mul r0.xyz, r0.x, c1
    add r0.w, r1.x, c0.x
    mad r0.xyz, r0.w, c0.z, r0
    add r0.w, r2.x, c0.y
    mad r0.xyz, r0.w, c2, r0
    mul r0.xyz, r0, t1
    mov r0.w, c0.w
    mul r0, r0, t1.w
    mad r0, t0, r0.w, r0
    mov oC0, r0

// approximately 14 instruction slots used (3 texture, 11 arithmetic)
ps_4_0
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
dcl_temps 2
sample r0.xyzw, v2.xyxx, t2.xyzw, s2
add r0.x, r0.x, l(-0.501961)
mul r0.xyz, r0.xxxx, l(1.596000, -0.813000, 0.000000, 0.000000)
sample r1.xyzw, v2.xyxx, t0.xyzw, s0
add r0.w, r1.x, l(-0.062745)
mad r0.xyz, r0.wwww, l(1.164000, 1.164000, 1.164000, 0.000000), r0.xyzx
sample r1.xyzw, v2.xyxx, t1.xyzw, s1
add r0.w, r1.x, l(-0.501961)
mad r0.xyz, r0.wwww, l(0.000000, -0.392000, 2.017000, 0.000000), r0.xyzx
mul r0.xyz, r0.xyzx, v1.xyzx
mov r0.w, l(1.000000)
mul r0.xyzw, r0.xyzw, v1.wwww
mad o0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
ret 
// Approximately 14 instruction slots used
#endif

const BYTE pBinary_D3D1xFL91_FYUVCxformAc[] =
{
     68,  88,  66,  67, 220,   6, 
    110, 201,  58, 156, 156,  11, 
     86, 125,   8,  39,  46, 124, 
     79, 201,   1,   0,   0,   0, 
    244,   5,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    236,   1,   0,   0,  68,   4, 
      0,   0, 192,   4,   0,   0, 
     92,   5,   0,   0, 192,   5, 
      0,   0,  65, 111, 110,  57, 
    172,   1,   0,   0, 172,   1, 
      0,   0,   0,   2, 255, 255, 
    124,   1,   0,   0,  48,   0, 
      0,   0,   0,   0,  48,   0, 
      0,   0,  48,   0,   0,   0, 
     48,   0,   3,   0,  36,   0, 
      0,   0,  48,   0,   0,   0, 
      0,   0,   1,   1,   1,   0, 
      2,   2,   2,   0,   0,   2, 
    255, 255,  81,   0,   0,   5, 
      0,   0,  15, 160, 129, 128, 
    128, 189, 129, 128,   0, 191, 
    244, 253, 148,  63,   0,   0, 
    128,  63,  81,   0,   0,   5, 
      1,   0,  15, 160, 186,  73, 
    204,  63, 197,  32,  80, 191, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  81,   0,   0,   5, 
      2,   0,  15, 160,   0,   0, 
      0,   0,  57, 180, 200, 190, 
    135,  22,   1,  64,   0,   0, 
      0,   0,  31,   0,   0,   2, 
      0,   0,   0, 128,   0,   0, 
     15, 176,  31,   0,   0,   2, 
      0,   0,   0, 128,   1,   0, 
     15, 176,  31,   0,   0,   2, 
      0,   0,   0, 128,   2,   0, 
      3, 176,  31,   0,   0,   2, 
      0,   0,   0, 144,   0,   8, 
     15, 160,  31,   0,   0,   2, 
      0,   0,   0, 144,   1,   8, 
     15, 160,  31,   0,   0,   2, 
      0,   0,   0, 144,   2,   8, 
     15, 160,  66,   0,   0,   3, 
      0,   0,  15, 128,   2,   0, 
    228, 176,   2,   8, 228, 160, 
     66,   0,   0,   3,   1,   0, 
     15, 128,   2,   0, 228, 176, 
      0,   8, 228, 160,  66,   0, 
      0,   3,   2,   0,  15, 128, 
      2,   0, 228, 176,   1,   8, 
    228, 160,   2,   0,   0,   3, 
      0,   0,   1, 128,   0,   0, 
      0, 128,   0,   0,  85, 160, 
      5,   0,   0,   3,   0,   0, 
      7, 128,   0,   0,   0, 128, 
      1,   0, 228, 160,   2,   0, 
      0,   3,   0,   0,   8, 128, 
      1,   0,   0, 128,   0,   0, 
      0, 160,   4,   0,   0,   4, 
      0,   0,   7, 128,   0,   0, 
    255, 128,   0,   0, 170, 160, 
      0,   0, 228, 128,   2,   0, 
      0,   3,   0,   0,   8, 128, 
      2,   0,   0, 128,   0,   0, 
     85, 160,   4,   0,   0,   4, 
      0,   0,   7, 128,   0,   0, 
    255, 128,   2,   0, 228, 160, 
      0,   0, 228, 128,   5,   0, 
      0,   3,   0,   0,   7, 128, 
      0,   0, 228, 128,   1,   0, 
    228, 176,   1,   0,   0,   2, 
      0,   0,   8, 128,   0,   0, 
    255, 160,   5,   0,   0,   3, 
      0,   0,  15, 128,   0,   0, 
    228, 128,   1,   0, 255, 176, 
      4,   0,   0,   4,   0,   0, 
     15, 128,   0,   0, 228, 176, 
      0,   0, 255, 128,   0,   0, 
    228, 128,   1,   0,   0,   2, 
      0,   8,  15, 128,   0,   0, 
    228, 128, 255, 255,   0,   0, 
     83,  72,  68,  82,  80,   2, 
      0,   0,  64,   0,   0,   0, 
    148,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      1,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      2,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   1,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   2,   0,   0,   0, 
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
      2,   0,   0,   0,   0,  96, 
     16,   0,   2,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128,   0, 191,  56,   0, 
      0,  10, 114,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 186,  73, 
    204,  63, 197,  32,  80, 191, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  69,   0,   0,   9, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0, 129, 128, 128, 189, 
     50,   0,   0,  12, 114,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    244, 253, 148,  63, 244, 253, 
    148,  63, 244, 253, 148,  63, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   2,   0, 
      0,   0,  70, 126,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
    129, 128,   0, 191,  50,   0, 
      0,  12, 114,   0,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,  57, 180, 200, 190, 
    135,  22,   1,  64,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     70,  18,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
    246,  31,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,  14,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      6,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
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
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     92,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      3,   0,   0,   0,  12,   0, 
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
