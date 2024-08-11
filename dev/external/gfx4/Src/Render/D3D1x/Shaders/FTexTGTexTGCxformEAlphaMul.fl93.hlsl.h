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
// tex                               sampler      NA          NA    0        2
// tex                               texture  float4          2d    0        2
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   x  w
// TEXCOORD                 0   xyzw        1     NONE   float   xyzw
// TEXCOORD                 1   xyzw        2     NONE   float   xyzw
// TEXCOORD                 2   xy          3     NONE   float   xy  
// TEXCOORD                 3     zw        3     NONE   float     zw
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
//
//
// Level9 shader bytecode:
//
    ps_2_x
    dcl t0
    dcl t1
    dcl t2
    dcl_pp t3
    dcl_2d s0
    dcl_2d s1
    mov_pp r0.xy, t3.wzzw
    texld r1, t3, s0
    texld r0, r0, s1
    lrp r2, t0.x, r1, r0
    mov r0, t2
    mad r0, r2, r0, t1
    mul r1.w, r0.w, t0.w
    mul r1.xyz, r0, r1.w
    mov oC0, r1

// approximately 9 instruction slots used (2 texture, 7 arithmetic)
ps_4_0
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xy
dcl_input_ps linear v3.zw
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v3.xyxx, t0.xyzw, s0
sample r1.xyzw, v3.zwzz, t1.xyzw, s1
add r0.xyzw, r0.xyzw, -r1.xyzw
mad r0.xyzw, v0.xxxx, r0.xyzw, r1.xyzw
mad r0.xyzw, r0.xyzw, v2.xyzw, v1.xyzw
mul r0.w, r0.w, v0.w
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
// Approximately 9 instruction slots used
#endif

const BYTE pBinary_D3D1xFL93_FTexTGTexTGCxformEAlphaMul[] =
{
     68,  88,  66,  67,  66,  54, 
     81,   9, 160,  69,  49, 212, 
    208, 172, 255, 219, 212, 110, 
    230,  91,   1,   0,   0,   0, 
    196,   4,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
     72,   1,   0,   0, 224,   2, 
      0,   0,  92,   3,   0,   0, 
    248,   3,   0,   0, 144,   4, 
      0,   0,  65, 111, 110,  57, 
      8,   1,   0,   0,   8,   1, 
      0,   0,   0,   2, 255, 255, 
    220,   0,   0,   0,  44,   0, 
      0,   0,   0,   0,  44,   0, 
      0,   0,  44,   0,   0,   0, 
     44,   0,   2,   0,  36,   0, 
      0,   0,  44,   0,   0,   0, 
      0,   0,   1,   1,   1,   0, 
      1,   2, 255, 255,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      0,   0,  15, 176,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      1,   0,  15, 176,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      2,   0,  15, 176,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      3,   0,  47, 176,  31,   0, 
      0,   2,   0,   0,   0, 144, 
      0,   8,  15, 160,  31,   0, 
      0,   2,   0,   0,   0, 144, 
      1,   8,  15, 160,   1,   0, 
      0,   2,   0,   0,  35, 128, 
      3,   0, 235, 176,  66,   0, 
      0,   3,   1,   0,  15, 128, 
      3,   0, 228, 176,   0,   8, 
    228, 160,  66,   0,   0,   3, 
      0,   0,  15, 128,   0,   0, 
    228, 128,   1,   8, 228, 160, 
     18,   0,   0,   4,   2,   0, 
     15, 128,   0,   0,   0, 176, 
      1,   0, 228, 128,   0,   0, 
    228, 128,   1,   0,   0,   2, 
      0,   0,  15, 128,   2,   0, 
    228, 176,   4,   0,   0,   4, 
      0,   0,  15, 128,   2,   0, 
    228, 128,   0,   0, 228, 128, 
      1,   0, 228, 176,   5,   0, 
      0,   3,   1,   0,   8, 128, 
      0,   0, 255, 128,   0,   0, 
    255, 176,   5,   0,   0,   3, 
      1,   0,   7, 128,   0,   0, 
    228, 128,   1,   0, 255, 128, 
      1,   0,   0,   2,   0,   8, 
     15, 128,   1,   0, 228, 128, 
    255, 255,   0,   0,  83,  72, 
     68,  82, 144,   1,   0,   0, 
     64,   0,   0,   0, 100,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 146,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      2,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      3,   0,   0,   0,  98,  16, 
      0,   3, 194,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   3,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  26,  16,   0,   3,   0, 
      0,   0,  70, 126,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   8, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  14,  16, 128, 
     65,   0,   0,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,   6,  16,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
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
      9,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  82,  68,  69,  70, 
    148,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  28,   0, 
      0,   0,   0,   4, 255, 255, 
      0, 145,   0,   0,  96,   0, 
      0,   0,  92,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      1,   0,   0,   0,  92,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  12,   0,   0,   0, 
    116, 101, 120,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  57,  46,  51, 
     48,  46,  57,  50,  48,  48, 
     46,  50,  48,  52,  57,  57, 
      0, 171,  73,  83,  71,  78, 
    144,   0,   0,   0,   5,   0, 
      0,   0,   8,   0,   0,   0, 
    128,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   9,   0,   0, 
    134,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
    134,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,  15,   0,   0, 
    134,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,   3,   3,   0,   0, 
    134,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  12,  12,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171
};
