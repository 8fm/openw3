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
//   float4x4 vfmuniforms[24];          // Offset:    0 Size:  1536
//   float4 vfuniforms[48];             // Offset: 1536 Size:   768
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
// COLOR                    1   xyzw        1     NONE   float   xyzw
// POSITION                 0   xyzw        2     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   xyzw
// COLOR                    1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 0   xy          2     NONE   float   xy  
// SV_Position              0   xyzw        3      POS   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c0         cb0             0       144  ( FLT, FLT, FLT, FLT)
//
//
// Runtime generated constant mappings:
//
// Target Reg                               Constant Description
// ---------- --------------------------------------------------
// c144                            Vertex Shader position offset
//
//
// Level9 shader bytecode:
//
    vs_2_0
    def c145, 255.009995, 510.019989, 0.100000001, 4
    def c146, 1.10000002, 0, 0, 0
    dcl_texcoord v0
    dcl_texcoord1 v1
    dcl_texcoord2 v2
    mad r0.xy, v1.z, c145, c145.z
    frc r0.zw, r0.xyxy
    add r0.xy, r0, -r0.zwzw
    mad r0.z, v1.z, c145.y, -r0.w
    add r0.z, r0.z, c146.x
    mova a0.x, r0.z
    dp4 oT2.y, v2, c96[a0.x]
    mul r0.x, r0.x, c145.w
    mova a0.x, r0.y
    dp4 oT2.x, v2, c96[a0.x]
    mova a0.x, r0.x
    dp4 oPos.z, v2, c2[a0.x]
    dp4 r0.x, v2, c0[a0.x]
    dp4 r0.y, v2, c1[a0.x]
    dp4 r0.z, v2, c3[a0.x]
    mad oPos.xy, r0.z, c144, r0
    mov oPos.w, r0.z
    mov oT0, v0
    mov oT1, v1

// approximately 19 instruction slots used
vs_4_0
dcl_constantbuffer cb0[144], dynamicIndexed
dcl_input v0.xyzw
dcl_input v1.xyzw
dcl_input v2.xyzw
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_output o2.xy
dcl_output_siv o3.xyzw, position
dcl_temps 1
mov o0.xyzw, v0.xyzw
mov o1.xyzw, v1.xyzw
mad r0.xyz, v1.zzzz, l(255.009995, 510.019989, 510.019989, 0.000000), l(0.100000, 0.100000, 1.100000, 0.000000)
ftou r0.xyz, r0.xyzx
dp4 o2.x, v2.xyzw, cb0[r0.y + 96].xyzw
dp4 o2.y, v2.xyzw, cb0[r0.z + 96].xyzw
ishl r0.x, r0.x, l(2)
dp4 o3.x, v2.xyzw, cb0[r0.x + 0].xyzw
dp4 o3.y, v2.xyzw, cb0[r0.x + 1].xyzw
dp4 o3.z, v2.xyzw, cb0[r0.x + 2].xyzw
dp4 o3.w, v2.xyzw, cb0[r0.x + 3].xyzw
ret 
// Approximately 12 instruction slots used
#endif

const BYTE pBinary_D3D1xFL91_VBatchPosition3dTexTGVertex[] =
{
     68,  88,  66,  67,  83, 215, 
    195,  28, 212, 110, 143, 230, 
    127, 105, 248, 157, 126,  53, 
    197, 122,   1,   0,   0,   0, 
    128,   6,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
      8,   2,   0,   0,  12,   4, 
      0,   0, 136,   4,   0,   0, 
    140,   5,   0,   0, 244,   5, 
      0,   0,  65, 111, 110,  57, 
    200,   1,   0,   0, 200,   1, 
      0,   0,   0,   2, 254, 255, 
    148,   1,   0,   0,  52,   0, 
      0,   0,   1,   0,  36,   0, 
      0,   0,  48,   0,   0,   0, 
     48,   0,   0,   0,  36,   0, 
      1,   0,  48,   0,   0,   0, 
      0,   0, 144,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    144,   0,   0,   2, 254, 255, 
     81,   0,   0,   5, 145,   0, 
     15, 160, 143,   2, 127,  67, 
    143,   2, 255,  67, 205, 204, 
    204,  61,   0,   0, 128,  64, 
     81,   0,   0,   5, 146,   0, 
     15, 160, 205, 204, 140,  63, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   5,   0, 
      0, 128,   0,   0,  15, 144, 
     31,   0,   0,   2,   5,   0, 
      1, 128,   1,   0,  15, 144, 
     31,   0,   0,   2,   5,   0, 
      2, 128,   2,   0,  15, 144, 
      4,   0,   0,   4,   0,   0, 
      3, 128,   1,   0, 170, 144, 
    145,   0, 228, 160, 145,   0, 
    170, 160,  19,   0,   0,   2, 
      0,   0,  12, 128,   0,   0, 
     68, 128,   2,   0,   0,   3, 
      0,   0,   3, 128,   0,   0, 
    228, 128,   0,   0, 238, 129, 
      4,   0,   0,   4,   0,   0, 
      4, 128,   1,   0, 170, 144, 
    145,   0,  85, 160,   0,   0, 
    255, 129,   2,   0,   0,   3, 
      0,   0,   4, 128,   0,   0, 
    170, 128, 146,   0,   0, 160, 
     46,   0,   0,   2,   0,   0, 
      1, 176,   0,   0, 170, 128, 
      9,   0,   0,   4,   2,   0, 
      2, 224,   2,   0, 228, 144, 
     96,  32, 228, 160,   0,   0, 
      0, 176,   5,   0,   0,   3, 
      0,   0,   1, 128,   0,   0, 
      0, 128, 145,   0, 255, 160, 
     46,   0,   0,   2,   0,   0, 
      1, 176,   0,   0,  85, 128, 
      9,   0,   0,   4,   2,   0, 
      1, 224,   2,   0, 228, 144, 
     96,  32, 228, 160,   0,   0, 
      0, 176,  46,   0,   0,   2, 
      0,   0,   1, 176,   0,   0, 
      0, 128,   9,   0,   0,   4, 
      0,   0,   4, 192,   2,   0, 
    228, 144,   2,  32, 228, 160, 
      0,   0,   0, 176,   9,   0, 
      0,   4,   0,   0,   1, 128, 
      2,   0, 228, 144,   0,  32, 
    228, 160,   0,   0,   0, 176, 
      9,   0,   0,   4,   0,   0, 
      2, 128,   2,   0, 228, 144, 
      1,  32, 228, 160,   0,   0, 
      0, 176,   9,   0,   0,   4, 
      0,   0,   4, 128,   2,   0, 
    228, 144,   3,  32, 228, 160, 
      0,   0,   0, 176,   4,   0, 
      0,   4,   0,   0,   3, 192, 
      0,   0, 170, 128, 144,   0, 
    228, 160,   0,   0, 228, 128, 
      1,   0,   0,   2,   0,   0, 
      8, 192,   0,   0, 170, 128, 
      1,   0,   0,   2,   0,   0, 
     15, 224,   0,   0, 228, 144, 
      1,   0,   0,   2,   1,   0, 
     15, 224,   1,   0, 228, 144, 
    255, 255,   0,   0,  83,  72, 
     68,  82, 252,   1,   0,   0, 
     64,   0,   1,   0, 127,   0, 
      0,   0,  89,   8,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0, 144,   0,   0,   0, 
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
    101,   0,   0,   3,  50,  32, 
     16,   0,   2,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   3,   0,   0,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     54,   0,   0,   5, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    242,  32,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,  15, 114,   0,  16,   0, 
      0,   0,   0,   0, 166,  26, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0, 143,   2, 
    127,  67, 143,   2, 255,  67, 
    143,   2, 255,  67,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    205, 204, 204,  61, 205, 204, 
    204,  61, 205, 204, 140,  63, 
      0,   0,   0,   0,  28,   0, 
      0,   5, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,  10,  18,  32, 
     16,   0,   2,   0,   0,   0, 
     70,  30,  16,   0,   2,   0, 
      0,   0,  70, 142,  32,   6, 
      0,   0,   0,   0,  96,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,  10,  34,  32,  16,   0, 
      2,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,  96,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  41,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   2,   0,   0,   0, 
     17,   0,   0,   9,  18,  32, 
     16,   0,   3,   0,   0,   0, 
     70,  30,  16,   0,   2,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,  10,  34,  32, 
     16,   0,   3,   0,   0,   0, 
     70,  30,  16,   0,   2,   0, 
      0,   0,  70, 142,  32,   6, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,  10,  66,  32,  16,   0, 
      3,   0,   0,   0,  70,  30, 
     16,   0,   2,   0,   0,   0, 
     70, 142,  32,   6,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,  10, 
    130,  32,  16,   0,   3,   0, 
      0,   0,  70,  30,  16,   0, 
      2,   0,   0,   0,  70, 142, 
     32,   6,   0,   0,   0,   0, 
      3,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 116,   0,   0,   0, 
     12,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      7,   0,   0,   0,   6,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  82,  68,  69,  70, 
    252,   0,   0,   0,   1,   0, 
      0,   0,  72,   0,   0,   0, 
      1,   0,   0,   0,  28,   0, 
      0,   0,   0,   4, 254, 255, 
      0, 145,   0,   0, 200,   0, 
      0,   0,  60,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,  36,  71, 
    108, 111,  98,  97, 108, 115, 
      0, 171, 171, 171,  60,   0, 
      0,   0,   2,   0,   0,   0, 
     96,   0,   0,   0,   0,   9, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 144,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   6,   0,   0,   2,   0, 
      0,   0, 156,   0,   0,   0, 
      0,   0,   0,   0, 172,   0, 
      0,   0,   0,   6,   0,   0, 
      0,   3,   0,   0,   2,   0, 
      0,   0, 184,   0,   0,   0, 
      0,   0,   0,   0, 118, 102, 
    109, 117, 110, 105, 102, 111, 
    114, 109, 115,   0,   3,   0, 
      3,   0,   4,   0,   4,   0, 
     24,   0,   0,   0,   0,   0, 
      0,   0, 118, 102, 117, 110, 
    105, 102, 111, 114, 109, 115, 
      0, 171,   1,   0,   3,   0, 
      1,   0,   4,   0,  48,   0, 
      0,   0,   0,   0,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  57, 
     46,  51,  48,  46,  57,  50, 
     48,  48,  46,  50,  48,  52, 
     57,  57,   0, 171,  73,  83, 
     71,  78,  96,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  80,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0,  86,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,  15,  15, 
      0,   0,  67,  79,  76,  79, 
     82,   0,  80,  79,  83,  73, 
     84,  73,  79,  78,   0, 171, 
     79,  83,  71,  78, 132,   0, 
      0,   0,   4,   0,   0,   0, 
      8,   0,   0,   0, 104,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   0,   0,   0, 110,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
      3,  12,   0,   0, 119,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   3,   0,   0,   0, 
     15,   0,   0,   0,  67,  79, 
     76,  79,  82,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0,  83,  86,  95,  80, 111, 
    115, 105, 116, 105, 111, 110, 
      0, 171
};
