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
//   float4 vfuniforms[144];            // Offset:    0 Size:  2304
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// SV_Position              0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 0   xy          1     NONE   float   xy  
// TEXCOORD                 1     zw        1     NONE   float     zw
// SV_Position              0   xyzw        2      POS   float   xyzw
//
vs_4_0
dcl_constantbuffer cb0[144], dynamicIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output o0.xyzw
dcl_output o1.xy
dcl_output o1.zw
dcl_output_siv o2.xyzw, position
dcl_temps 1
mov o0.xyzw, v0.xyzw
mad r0.xyzw, v0.zzzz, l(1530.059937, 1530.059937, 1530.059937, 1530.059937), l(0.100000, 1.100000, 2.100000, 3.100000)
ftou r0.xyzw, r0.xyzw
dp4 o1.x, v1.xyzw, cb0[r0.z + 0].xyzw
dp4 o1.y, v1.xyzw, cb0[r0.w + 0].xyzw
mad r0.zw, v0.zzzz, l(0.000000, 0.000000, 1530.059937, 1530.059937), l(0.000000, 0.000000, 4.100000, 5.100000)
ftou r0.zw, r0.zzzw
dp4 o1.z, v1.xyzw, cb0[r0.z + 0].xyzw
dp4 o1.w, v1.xyzw, cb0[r0.w + 0].xyzw
dp4 o2.x, v1.xyzw, cb0[r0.x + 0].xyzw
dp4 o2.y, v1.xyzw, cb0[r0.y + 0].xyzw
mov o2.zw, l(0,0,0,1.000000)
ret 
// Approximately 13 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_VBatchTexTGTexTG[] =
{
     68,  88,  66,  67,  90, 145, 
    114, 163, 150,  25, 162, 184, 
     42, 178,  25, 149,  95,  47, 
     14,  67,   1,   0,   0,   0, 
    132,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      4,   1,   0,   0,  88,   1, 
      0,   0, 228,   1,   0,   0, 
      8,   4,   0,   0,  82,  68, 
     69,  70, 200,   0,   0,   0, 
      1,   0,   0,   0,  72,   0, 
      0,   0,   1,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    254, 255,   0, 129,   0,   0, 
    148,   0,   0,   0,  60,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0, 171, 171, 
     60,   0,   0,   0,   1,   0, 
      0,   0,  96,   0,   0,   0, 
      0,   9,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    120,   0,   0,   0,   0,   0, 
      0,   0,   0,   9,   0,   0, 
      2,   0,   0,   0, 132,   0, 
      0,   0,   0,   0,   0,   0, 
    118, 102, 117, 110, 105, 102, 
    111, 114, 109, 115,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0, 144,   0,   0,   0, 
      0,   0,   0,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  57,  46,  51, 
     48,  46,  57,  50,  48,  48, 
     46,  50,  48,  52,  57,  57, 
      0, 171,  73,  83,  71,  78, 
     76,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     62,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     83,  86,  95,  80, 111, 115, 
    105, 116, 105, 111, 110,   0, 
    171, 171,  79,  83,  71,  78, 
    132,   0,   0,   0,   4,   0, 
      0,   0,   8,   0,   0,   0, 
    104,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
    110,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   3,  12,   0,   0, 
    110,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  12,   3,   0,   0, 
    119,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,   0,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171,  83,  72, 
     68,  82,  28,   2,   0,   0, 
     64,   0,   1,   0, 135,   0, 
      0,   0,  89,   8,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0, 144,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    101,   0,   0,   3,  50,  32, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 194,  32, 
     16,   0,   1,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   2,   0,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     54,   0,   0,   5, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,  15, 
    242,   0,  16,   0,   0,   0, 
      0,   0, 166,  26,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0, 235,  65, 191,  68, 
    235,  65, 191,  68, 235,  65, 
    191,  68, 235,  65, 191,  68, 
      2,  64,   0,   0, 205, 204, 
    204,  61, 205, 204, 140,  63, 
    102, 102,   6,  64, 102, 102, 
     70,  64,  28,   0,   0,   5, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   9,  18,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   4,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   9,  34,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   4,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  15, 194,   0,  16,   0, 
      0,   0,   0,   0, 166,  26, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    235,  65, 191,  68, 235,  65, 
    191,  68,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  51,  51, 131,  64, 
     51,  51, 163,  64,  28,   0, 
      0,   5, 194,   0,  16,   0, 
      0,   0,   0,   0, 166,  14, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   9,  66,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   9, 130,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   9,  18,  32, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   9,  34,  32, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   8, 194,  32, 
     16,   0,   2,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    128,  63,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,  13,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
      6,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
