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
//   float4x4 mvp;                      // Offset:    0 Size:    64
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
// TEXCOORD                 0   xy          0     NONE   float   xy  
// SV_Position              0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// SV_Position              0   xyzw        1      POS   float   xyzw
//
vs_4_0
dcl_constantbuffer cb0[4], immediateIndexed
dcl_input v0.xy
dcl_input v1.xyzw
dcl_output o0.xy
dcl_output_siv o1.xyzw, position
mov o0.xy, v0.xyxx
dp4 o1.x, v1.xyzw, cb0[0].xyzw
dp4 o1.y, v1.xyzw, cb0[1].xyzw
dp4 o1.z, v1.xyzw, cb0[2].xyzw
dp4 o1.w, v1.xyzw, cb0[3].xyzw
ret 
// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL10X_VPosition3dTexUV[] =
{
     68,  88,  66,  67, 216, 178, 
     23, 118, 185, 104, 149, 240, 
     73, 107, 241,  98, 253,   5, 
    254, 137,   1,   0,   0,   0, 
     20,   3,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    252,   0,   0,   0,  84,   1, 
      0,   0, 172,   1,   0,   0, 
    152,   2,   0,   0,  82,  68, 
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
     64,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    120,   0,   0,   0,   0,   0, 
      0,   0,  64,   0,   0,   0, 
      2,   0,   0,   0, 124,   0, 
      0,   0,   0,   0,   0,   0, 
    109, 118, 112,   0,   3,   0, 
      3,   0,   4,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  57,  46,  51,  48,  46, 
     57,  50,  48,  48,  46,  50, 
     48,  52,  57,  57,   0, 171, 
     73,  83,  71,  78,  80,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   3,   0,   0,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,  15,   0,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0,  83,  86,  95,  80, 111, 
    115, 105, 116, 105, 111, 110, 
      0, 171, 171, 171,  79,  83, 
     71,  78,  80,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   3,  12, 
      0,   0,  65,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,   0, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0,  83, 
     86,  95,  80, 111, 115, 105, 
    116, 105, 111, 110,   0, 171, 
    171, 171,  83,  72,  68,  82, 
    228,   0,   0,   0,  64,   0, 
      1,   0,  57,   0,   0,   0, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  95,   0, 
      0,   3,  50,  16,  16,   0, 
      0,   0,   0,   0,  95,   0, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3,  50,  32,  16,   0, 
      0,   0,   0,   0, 103,   0, 
      0,   4, 242,  32,  16,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
     50,  32,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   8,  18,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     17,   0,   0,   8,  34,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  17,   0,   0,   8, 
     66,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  17,   0, 
      0,   8, 130,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
      6,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
