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
// TEXCOORD                 0   xy          1     NONE   float   xy  
// TEXCOORD                 1     zw        1     NONE   float     zw
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
    ps_2_0
    dcl t0
    dcl_pp t1
    dcl_2d s0
    dcl_2d s1
    mov_pp r0.xy, t1.wzyx
    texld r0, r0, s1
    texld r1, t1, s0
    lrp r2.w, t0.x, r1.w, r0.w
    mul r0, r2.w, t0.w
    mov oC0, r0

// approximately 6 instruction slots used (2 texture, 4 arithmetic)
ps_4_0
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xw
dcl_input_ps linear v1.xy
dcl_input_ps linear v1.zw
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v1.xyxx, t0.xyzw, s0
sample r1.xyzw, v1.zwzz, t1.xyzw, s1
add r0.x, r0.w, -r1.w
mad r0.x, v0.x, r0.x, r1.w
mul o0.xyzw, r0.xxxx, v0.wwww
ret 
// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL91_FTexTGTexTGEAlphaInv[] =
{
     68,  88,  66,  67, 106,  42, 
    150, 118, 231,   3, 122,  48, 
     53, 204,  62,  50, 150, 127, 
    216, 198,   1,   0,   0,   0, 
    224,   3,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   1,   0,   0,  44,   2, 
      0,   0, 168,   2,   0,   0, 
     68,   3,   0,   0, 172,   3, 
      0,   0,  65, 111, 110,  57, 
    192,   0,   0,   0, 192,   0, 
      0,   0,   0,   2, 255, 255, 
    148,   0,   0,   0,  44,   0, 
      0,   0,   0,   0,  44,   0, 
      0,   0,  44,   0,   0,   0, 
     44,   0,   2,   0,  36,   0, 
      0,   0,  44,   0,   0,   0, 
      0,   0,   1,   1,   1,   0, 
      0,   2, 255, 255,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      0,   0,  15, 176,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      1,   0,  47, 176,  31,   0, 
      0,   2,   0,   0,   0, 144, 
      0,   8,  15, 160,  31,   0, 
      0,   2,   0,   0,   0, 144, 
      1,   8,  15, 160,   1,   0, 
      0,   2,   0,   0,  35, 128, 
      1,   0,  27, 176,  66,   0, 
      0,   3,   0,   0,  15, 128, 
      0,   0, 228, 128,   1,   8, 
    228, 160,  66,   0,   0,   3, 
      1,   0,  15, 128,   1,   0, 
    228, 176,   0,   8, 228, 160, 
     18,   0,   0,   4,   2,   0, 
      8, 128,   0,   0,   0, 176, 
      1,   0, 255, 128,   0,   0, 
    255, 128,   5,   0,   0,   3, 
      0,   0,  15, 128,   2,   0, 
    255, 128,   0,   0, 255, 176, 
      1,   0,   0,   2,   0,   8, 
     15, 128,   0,   0, 228, 128, 
    255, 255,   0,   0,  83,  72, 
     68,  82,  36,   1,   0,   0, 
     64,   0,   0,   0,  73,   0, 
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
      0,   3,  50,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 194,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   1,   0,   0,   0, 
    230,  26,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16, 128, 
     65,   0,   0,   0,   1,   0, 
      0,   0,  50,   0,   0,   9, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,  16,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,  32,  16,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
      6,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
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
     96,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   9,   0,   0, 
     86,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   3,   3,   0,   0, 
     86,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
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
