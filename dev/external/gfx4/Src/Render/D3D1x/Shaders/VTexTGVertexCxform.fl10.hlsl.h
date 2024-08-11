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
//   float4 cxadd;                      // Offset:    0 Size:    16
//   float4 cxmul;                      // Offset:   16 Size:    16
//   float4 mvp[2];                     // Offset:   32 Size:    32
//   float4 texgen[2];                  // Offset:   64 Size:    32
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
// COLOR                    1   xyzw        1     NONE   float   xyzw
// SV_Position              0   xyzw        2     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 0   xyzw        2     NONE   float   xyzw
// TEXCOORD                 1   xyzw        3     NONE   float   xyzw
// TEXCOORD                 2   xy          4     NONE   float   xy  
// SV_Position              0   xyzw        5      POS   float   xyzw
//
vs_4_0
dcl_constantbuffer cb0[6], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_input v2.xyzw
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_output o2.xyzw
dcl_output o3.xyzw
dcl_output o4.xy
dcl_output_siv o5.xyzw, position
mov o0.xyzw, v0.xyzw
mov o1.xyzw, v1.xyzw
mov o2.xyzw, cb0[0].xyzw
mov o3.xyzw, cb0[1].xyzw
dp4 o4.x, v2.xyzw, cb0[4].xyzw
dp4 o4.y, v2.xyzw, cb0[5].xyzw
dp4 o5.x, v2.xyzw, cb0[2].xyzw
dp4 o5.y, v2.xyzw, cb0[3].xyzw
mov o5.zw, l(0,0,0,1.000000)
ret 
// Approximately 10 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_VTexTGVertexCxform[] =
{
     68,  88,  66,  67, 136, 255, 
    144, 167, 171, 179, 228, 248, 
    160, 137,   4, 211,  67,  97, 
      1,  19,   1,   0,   0,   0, 
    172,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    124,   1,   0,   0, 232,   1, 
      0,   0, 164,   2,   0,   0, 
     48,   4,   0,   0,  82,  68, 
     69,  70,  64,   1,   0,   0, 
      1,   0,   0,   0,  72,   0, 
      0,   0,   1,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    254, 255,   0, 129,   0,   0, 
     12,   1,   0,   0,  60,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0, 171, 171, 
     60,   0,   0,   0,   4,   0, 
      0,   0,  96,   0,   0,   0, 
     96,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    192,   0,   0,   0,   0,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 200,   0, 
      0,   0,   0,   0,   0,   0, 
    216,   0,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0, 200,   0, 
      0,   0,   0,   0,   0,   0, 
    222,   0,   0,   0,  32,   0, 
      0,   0,  32,   0,   0,   0, 
      2,   0,   0,   0, 228,   0, 
      0,   0,   0,   0,   0,   0, 
    244,   0,   0,   0,  64,   0, 
      0,   0,  32,   0,   0,   0, 
      2,   0,   0,   0, 252,   0, 
      0,   0,   0,   0,   0,   0, 
     99, 120,  97, 100, 100,   0, 
    171, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     99, 120, 109, 117, 108,   0, 
    109, 118, 112,   0, 171, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   2,   0,   0,   0, 
      0,   0,   0,   0, 116, 101, 
    120, 103, 101, 110,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   2,   0,   0,   0, 
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
    100,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     86,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,  15,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     83,  86,  95,  80, 111, 115, 
    105, 116, 105, 111, 110,   0, 
    171, 171,  79,  83,  71,  78, 
    180,   0,   0,   0,   6,   0, 
      0,   0,   8,   0,   0,   0, 
    152,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
    152,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   4,   0, 
      0,   0,   3,  12,   0,   0, 
    167,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   5,   0, 
      0,   0,  15,   0,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171,  83,  72, 
     68,  82, 132,   1,   0,   0, 
     64,   0,   1,   0,  97,   0, 
      0,   0,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   3,   0,   0,   0, 
    101,   0,   0,   3,  50,  32, 
     16,   0,   4,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   5,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 242,  32,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 242,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   6, 
    242,  32,  16,   0,   2,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   6, 
    242,  32,  16,   0,   3,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  17,   0,   0,   8, 
     18,  32,  16,   0,   4,   0, 
      0,   0,  70,  30,  16,   0, 
      2,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  17,   0, 
      0,   8,  34,  32,  16,   0, 
      4,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
     17,   0,   0,   8,  18,  32, 
     16,   0,   5,   0,   0,   0, 
     70,  30,  16,   0,   2,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  17,   0,   0,   8, 
     34,  32,  16,   0,   5,   0, 
      0,   0,  70,  30,  16,   0, 
      2,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  54,   0, 
      0,   8, 194,  32,  16,   0, 
      5,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 128,  63, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
     10,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      9,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
