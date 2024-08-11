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
// sampler_tex                       sampler      NA          NA    0        4
// tex                               texture  float4          2d    0        4
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xyzw        1     NONE   float      w
// TEXCOORD                 1   xyzw        2     NONE   float      w
// TEXCOORD                 2   xy          3     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_4_0
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.w
dcl_input_ps linear v1.w
dcl_input_ps linear v2.w
dcl_input_ps linear v3.xy
dcl_output o0.xyzw
dcl_temps 1
sample r0.xyzw, v3.xyxx, t3.xyzw, s3
mul r0.x, r0.x, v2.w
mad r0.x, v1.w, r0.x, r0.x
mul o0.xyzw, r0.xxxx, v0.wwww
ret 
// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_FYUVACxformAcEAlphaInv[] =
{
     68,  88,  66,  67,  89, 203, 
    193, 146,  59, 193, 153,  59, 
    137, 114, 150, 228,  77, 146, 
    180, 209,   1,   0,   0,   0, 
      0,   3,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    220,   0,   0,   0,  92,   1, 
      0,   0, 144,   1,   0,   0, 
    132,   2,   0,   0,  82,  68, 
     69,  70, 160,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0, 129,   0,   0, 
    108,   0,   0,   0,  92,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   1,   0,   0,   0, 
    104,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      4,   0,   0,   0,  13,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 116, 101, 
    120,   0, 116, 101, 120,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  57, 
     46,  51,  48,  46,  57,  50, 
     48,  48,  46,  50,  48,  52, 
     57,  57,   0, 171,  73,  83, 
     71,  78, 120,   0,   0,   0, 
      4,   0,   0,   0,   8,   0, 
      0,   0, 104,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   8, 
      0,   0, 110,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,   8, 
      0,   0, 110,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,  15,   8, 
      0,   0, 110,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   3,   3, 
      0,   0,  67,  79,  76,  79, 
     82,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     68,  82, 236,   0,   0,   0, 
     64,   0,   0,   0,  59,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   3,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   3,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3, 130,  16, 
     16,   0,   2,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   3,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  69,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      3,   0,   0,   0,  70, 126, 
     16,   0,   3,   0,   0,   0, 
      0,  96,  16,   0,   3,   0, 
      0,   0,  56,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  58,  16, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,   9,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58,  16,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 242,  32, 
     16,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  83,  84,  65,  84, 
    116,   0,   0,   0,   5,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
