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
//   float4 cxadd;                      // Offset:    0 Size:    16
//   float4 cxmul;                      // Offset:   16 Size:    16
//   float4 mvp[2];                     // Offset:   32 Size:    32
//   float4 texgen[4];                  // Offset:   64 Size:    64
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
// TEXCOORD                 0   xyzw        1     NONE   float   xyzw
// TEXCOORD                 1   xyzw        2     NONE   float   xyzw
// TEXCOORD                 2   xy          3     NONE   float   xy  
// TEXCOORD                 3     zw        3     NONE   float     zw
// SV_Position              0   xyzw        4      POS   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c1         cb0             0         8  ( FLT, FLT, FLT, FLT)
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
    vs_2_0
    def c9, 0, 1, 0, 0
    dcl_texcoord v0
    dcl_texcoord1 v1
    dp4 oT3.x, v1, c5
    dp4 oT3.y, v1, c6
    dp4 oT3.w, v1, c7
    dp4 oT3.z, v1, c8
    dp4 r0.x, v1, c3
    dp4 r0.y, v1, c4
    add oPos.xy, r0, c0
    mov oT0, v0
    mov oT1, c1
    mov oT2, c2
    mov oPos.zw, c9.xyxy

// approximately 11 instruction slots used
vs_4_0
dcl_constantbuffer cb0[8], immediateIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_output o2.xyzw
dcl_output o3.xy
dcl_output o3.zw
dcl_output_siv o4.xyzw, position
mov o0.xyzw, v0.xyzw
mov o1.xyzw, cb0[0].xyzw
mov o2.xyzw, cb0[1].xyzw
dp4 o3.x, v1.xyzw, cb0[4].xyzw
dp4 o3.y, v1.xyzw, cb0[5].xyzw
dp4 o3.z, v1.xyzw, cb0[6].xyzw
dp4 o3.w, v1.xyzw, cb0[7].xyzw
dp4 o4.x, v1.xyzw, cb0[2].xyzw
dp4 o4.y, v1.xyzw, cb0[3].xyzw
mov o4.zw, l(0,0,0,1.000000)
ret 
// Approximately 11 instruction slots used
#endif

const BYTE pBinary_D3D1xFL91_VTexTGTexTGCxform[] =
{
     68,  88,  66,  67, 241, 189, 
    138, 246, 237, 250,  27,   8, 
     15,  69, 193, 214, 206, 105, 
    137,  52,   1,   0,   0,   0, 
    200,   5,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
     76,   1,   0,   0, 248,   2, 
      0,   0, 116,   3,   0,   0, 
    188,   4,   0,   0,  12,   5, 
      0,   0,  65, 111, 110,  57, 
     12,   1,   0,   0,  12,   1, 
      0,   0,   0,   2, 254, 255, 
    216,   0,   0,   0,  52,   0, 
      0,   0,   1,   0,  36,   0, 
      0,   0,  48,   0,   0,   0, 
     48,   0,   0,   0,  36,   0, 
      1,   0,  48,   0,   0,   0, 
      0,   0,   8,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   2, 254, 255, 
     81,   0,   0,   5,   9,   0, 
     15, 160,   0,   0,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   5,   0, 
      0, 128,   0,   0,  15, 144, 
     31,   0,   0,   2,   5,   0, 
      1, 128,   1,   0,  15, 144, 
      9,   0,   0,   3,   3,   0, 
      1, 224,   1,   0, 228, 144, 
      5,   0, 228, 160,   9,   0, 
      0,   3,   3,   0,   2, 224, 
      1,   0, 228, 144,   6,   0, 
    228, 160,   9,   0,   0,   3, 
      3,   0,   8, 224,   1,   0, 
    228, 144,   7,   0, 228, 160, 
      9,   0,   0,   3,   3,   0, 
      4, 224,   1,   0, 228, 144, 
      8,   0, 228, 160,   9,   0, 
      0,   3,   0,   0,   1, 128, 
      1,   0, 228, 144,   3,   0, 
    228, 160,   9,   0,   0,   3, 
      0,   0,   2, 128,   1,   0, 
    228, 144,   4,   0, 228, 160, 
      2,   0,   0,   3,   0,   0, 
      3, 192,   0,   0, 228, 128, 
      0,   0, 228, 160,   1,   0, 
      0,   2,   0,   0,  15, 224, 
      0,   0, 228, 144,   1,   0, 
      0,   2,   1,   0,  15, 224, 
      1,   0, 228, 160,   1,   0, 
      0,   2,   2,   0,  15, 224, 
      2,   0, 228, 160,   1,   0, 
      0,   2,   0,   0,  12, 192, 
      9,   0,  68, 160, 255, 255, 
      0,   0,  83,  72,  68,  82, 
    164,   1,   0,   0,  64,   0, 
      1,   0, 105,   0,   0,   0, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      8,   0,   0,   0,  95,   0, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  95,   0, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      2,   0,   0,   0, 101,   0, 
      0,   3,  50,  32,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 194,  32,  16,   0, 
      3,   0,   0,   0, 103,   0, 
      0,   4, 242,  32,  16,   0, 
      4,   0,   0,   0,   1,   0, 
      0,   0,  54,   0,   0,   5, 
    242,  32,  16,   0,   0,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6, 242,  32,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6, 242,  32,  16,   0, 
      2,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   8,  18,  32,  16,   0, 
      3,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     17,   0,   0,   8,  34,  32, 
     16,   0,   3,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,  17,   0,   0,   8, 
     66,  32,  16,   0,   3,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,  17,   0, 
      0,   8, 130,  32,  16,   0, 
      3,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   7,   0,   0,   0, 
     17,   0,   0,   8,  18,  32, 
     16,   0,   4,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  17,   0,   0,   8, 
     34,  32,  16,   0,   4,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  54,   0, 
      0,   8, 194,  32,  16,   0, 
      4,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 128,  63, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
     11,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      8,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  82,  68,  69,  70, 
     64,   1,   0,   0,   1,   0, 
      0,   0,  72,   0,   0,   0, 
      1,   0,   0,   0,  28,   0, 
      0,   0,   0,   4, 254, 255, 
      0, 145,   0,   0,  12,   1, 
      0,   0,  60,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,  36,  71, 
    108, 111,  98,  97, 108, 115, 
      0, 171, 171, 171,  60,   0, 
      0,   0,   4,   0,   0,   0, 
     96,   0,   0,   0, 128,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 192,   0, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 200,   0,   0,   0, 
      0,   0,   0,   0, 216,   0, 
      0,   0,  16,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 200,   0,   0,   0, 
      0,   0,   0,   0, 222,   0, 
      0,   0,  32,   0,   0,   0, 
     32,   0,   0,   0,   2,   0, 
      0,   0, 228,   0,   0,   0, 
      0,   0,   0,   0, 244,   0, 
      0,   0,  64,   0,   0,   0, 
     64,   0,   0,   0,   2,   0, 
      0,   0, 252,   0,   0,   0, 
      0,   0,   0,   0,  99, 120, 
     97, 100, 100,   0, 171, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  99, 120, 
    109, 117, 108,   0, 109, 118, 
    112,   0, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0, 116, 101, 120, 103, 
    101, 110,   0, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      4,   0,   0,   0,   0,   0, 
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
    180,   0,   0,   0,   6,   0, 
      0,   0,   8,   0,   0,   0, 
    152,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  15,   0,   0,   0, 
    158,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,   3,  12,   0,   0, 
    158,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   3,   0, 
      0,   0,  12,   3,   0,   0, 
    167,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   4,   0, 
      0,   0,  15,   0,   0,   0, 
     67,  79,  76,  79,  82,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80, 111, 115, 105, 116, 105, 
    111, 110,   0, 171
};
