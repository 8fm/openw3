#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.20499
//
//
///
// Buffer Definitions: 
//
// cbuffer $Globals
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
// $Globals                          cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// POSITION                 0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// SV_Position              0   xyzw        1      POS   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c1         cb0             0         4  ( FLT, FLT, FLT, FLT)
//
//
// Runtime generated constant mappings:
//
// Target Reg                               Constant Description
// ---------- --------------------------------------------------
// c0                              Vertex Shader position offset
//
//
// Level9 shader bytecode:
//
    vs_2_x
    dcl_texcoord v0
    dcl_texcoord1 v1
    dp4 oPos.z, v1, c3
    dp4 r0.x, v1, c1
    dp4 r0.y, v1, c2
    dp4 r0.z, v1, c4
    mad oPos.xy, r0.z, c0, r0
    mov oPos.w, r0.z
    mov oT0, v0

// approximately 7 instruction slots used
vs_4_0
dcl_constantbuffer cb0[4], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output o0.xyzw
dcl_output_siv o1.xyzw, position
mov o0.xyzw, v0.xyzw
dp4 o1.x, v1.xyzw, cb0[0].xyzw
dp4 o1.y, v1.xyzw, cb0[1].xyzw
dp4 o1.z, v1.xyzw, cb0[2].xyzw
dp4 o1.w, v1.xyzw, cb0[3].xyzw
ret 
// Approximately 6 instruction slots used
#endif

const BYTE pBinary_D3D1xFL93_VPosition3dVertex[] =
{
     68,  88,  66,  67, 118,  98, 
     45,  30, 239,  37,  95,  21, 
     66,  84,  16,  20, 249, 181, 
    170,  43,   1,   0,   0,   0, 
    212,   3,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   1,   0,   0, 236,   1, 
      0,   0, 104,   2,   0,   0, 
     48,   3,   0,   0, 128,   3, 
      0,   0,  65, 111, 110,  57, 
    192,   0,   0,   0, 192,   0, 
      0,   0,   0,   2, 254, 255, 
    140,   0,   0,   0,  52,   0, 
      0,   0,   1,   0,  36,   0, 
      0,   0,  48,   0,   0,   0, 
     48,   0,   0,   0,  36,   0, 
      1,   0,  48,   0,   0,   0, 
      0,   0,   4,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   2, 254, 255, 
     31,   0,   0,   2,   5,   0, 
      0, 128,   0,   0,  15, 144, 
     31,   0,   0,   2,   5,   0, 
      1, 128,   1,   0,  15, 144, 
      9,   0,   0,   3,   0,   0, 
      4, 192,   1,   0, 228, 144, 
      3,   0, 228, 160,   9,   0, 
      0,   3,   0,   0,   1, 128, 
      1,   0, 228, 144,   1,   0, 
    228, 160,   9,   0,   0,   3, 
      0,   0,   2, 128,   1,   0, 
    228, 144,   2,   0, 228, 160, 
      9,   0,   0,   3,   0,   0, 
      4, 128,   1,   0, 228, 144, 
      4,   0, 228, 160,   4,   0, 
      0,   4,   0,   0,   3, 192, 
      0,   0, 170, 128,   0,   0, 
    228, 160,   0,   0, 228, 128, 
      1,   0,   0,   2,   0,   0, 
      8, 192,   0,   0, 170, 128, 
      1,   0,   0,   2,   0,   0, 
     15, 224,   0,   0, 228, 144, 
    255, 255,   0,   0,  83,  72, 
     68,  82, 228,   0,   0,   0, 
     64,   0,   1,   0,  57,   0, 
      0,   0,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     95,   0,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5, 242,  32,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   8,  18,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     34,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   8,  66,  32,  16,   0, 
      1,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     17,   0,   0,   8, 130,  32, 
     16,   0,   1,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,   6,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
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
     69,  70, 192,   0,   0,   0, 
      1,   0,   0,   0,  72,   0, 
      0,   0,   1,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    254, 255,   0, 145,   0,   0, 
    140,   0,   0,   0,  60,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
     36,  71, 108, 111,  98,  97, 
    108, 115,   0, 171, 171, 171, 
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
     73,  83,  71,  78,  72,   0, 
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
     76,  79,  82,   0,  80,  79, 
     83,  73,  84,  73,  79,  78, 
      0, 171,  79,  83,  71,  78, 
     76,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     62,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     83,  86,  95,  80, 111, 115, 
    105, 116, 105, 111, 110,   0, 
    171, 171
};
