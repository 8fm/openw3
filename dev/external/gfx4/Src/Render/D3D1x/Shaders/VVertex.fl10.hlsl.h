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
//   float4 mvp[2];                     // Offset:    0 Size:    32
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
// SV_Position              0   xyzw        1      POS   float   xyzw
//
vs_4_0
dcl_constantbuffer cb0[2], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output o0.xyzw
dcl_output_siv o1.xyzw, position
mov o0.xyzw, v0.xyzw
dp4 o1.x, v1.xyzw, cb0[0].xyzw
dp4 o1.y, v1.xyzw, cb0[1].xyzw
mov o1.zw, l(0,0,0,1.000000)
ret 
// Approximately 5 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_VVertex[] =
{
     68,  88,  66,  67, 170,  65, 
     48, 166, 144, 148,  40, 182, 
     15,  74, 135, 252,  36,  89, 
    231, 229,   1,   0,   0,   0, 
    236,   2,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    252,   0,   0,   0,  80,   1, 
      0,   0, 164,   1,   0,   0, 
    112,   2,   0,   0,  82,  68, 
     69,  70, 192,   0,   0,   0, 
      1,   0,   0,   0,  72,   0, 
      0,   0,   1,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    254, 255,   0, 129,   0,   0, 
    140,   0,   0,   0,  60,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0, 171, 171, 
     60,   0,   0,   0,   1,   0, 
      0,   0,  96,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    120,   0,   0,   0,   0,   0, 
      0,   0,  32,   0,   0,   0, 
      2,   0,   0,   0, 124,   0, 
      0,   0,   0,   0,   0,   0, 
    109, 118, 112,   0,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  57,  46,  51,  48,  46, 
     57,  50,  48,  48,  46,  50, 
     48,  52,  57,  57,   0, 171, 
     73,  83,  71,  78,  76,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,  15,   0,   0,  62,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,  15,   0,   0,  67,  79, 
     76,  79,  82,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0, 171, 171, 
     79,  83,  71,  78,  76,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  62,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   0,   0,   0,  67,  79, 
     76,  79,  82,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0, 171, 171, 
     83,  72,  68,  82, 196,   0, 
      0,   0,  64,   0,   1,   0, 
     49,   0,   0,   0,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   0,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 103,   0,   0,   4, 
    242,  32,  16,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     18,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   8,  34,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   8, 194,  32, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    128,  63,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,   5,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
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
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
