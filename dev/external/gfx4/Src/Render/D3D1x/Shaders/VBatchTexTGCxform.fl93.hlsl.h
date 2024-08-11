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
//   float4 vfuniforms[144];            // Offset:    0 Size:  2304
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
// POSITION                 0   xyzw        0     NONE   float   xyzw
// COLOR                    1   x           1     NONE   float   x   
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
    vs_2_x
    def c145, 0, 1, 0, 0
    def c146, 2.0999999, 4.0999999, 0.100000001, 1.10000002
    def c147, 1530.05994, 3.0999999, 5.0999999, 0
    dcl_texcoord v0
    dcl_texcoord1 v1
    mov r0.x, c147.x
    mad r0, v1.x, r0.x, c146
    frc r1, r0
    add r0, r0, -r1
    mad r1.xy, v1.x, c147.x, -r1
    add r1.xy, r1, c147.yzzw
    mova a0.x, r0.y
    dp4 oT2.x, v0, c0[a0.x]
    mova a0.xy, r1.yxzw
    dp4 r1.y, v0, c0[a0.y]
    dp4 oT2.y, v0, c0[a0.x]
    mova a0.x, r0.x
    dp4 r1.x, v0, c0[a0.x]
    add oPos.xy, r1, c144
    mova a0.xy, r0.zwzw
    mov oT1, c0[a0.y]
    mov oT0, c0[a0.x]
    mov oPos.zw, c145.xyxy

// approximately 18 instruction slots used
vs_4_0
dcl_constantbuffer cb0[144], dynamicIndexed
dcl_input v0.xyzw
dcl_input v1.x
dcl_output o0.xyzw
dcl_output o1.xyzw
dcl_output o2.xy
dcl_output_siv o3.xyzw, position
dcl_temps 1
mad r0.xy, v1.xxxx, l(1530.059937, 1530.059937, 0.000000, 0.000000), l(0.100000, 1.100000, 0.000000, 0.000000)
ftou r0.xy, r0.xyxx
mov o0.xyzw, cb0[r0.x + 0].xyzw
mov o1.xyzw, cb0[r0.y + 0].xyzw
mad r0.xyzw, v1.xxxx, l(1530.059937, 1530.059937, 1530.059937, 1530.059937), l(2.100000, 3.100000, 4.100000, 5.100000)
ftou r0.xyzw, r0.xyzw
dp4 o2.x, v0.xyzw, cb0[r0.z + 0].xyzw
dp4 o2.y, v0.xyzw, cb0[r0.w + 0].xyzw
dp4 o3.x, v0.xyzw, cb0[r0.x + 0].xyzw
dp4 o3.y, v0.xyzw, cb0[r0.y + 0].xyzw
mov o3.zw, l(0,0,0,1.000000)
ret 
// Approximately 12 instruction slots used
#endif

const BYTE pBinary_D3D1xFL93_VBatchTexTGCxform[] =
{
     68,  88,  66,  67, 245,  32, 
     54, 220, 155,  70, 205, 229, 
    157, 200, 158,  51, 110,  30, 
     43, 248,   1,   0,   0,   0, 
     28,   6,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    248,   1,   0,   0, 248,   3, 
      0,   0, 116,   4,   0,   0, 
     68,   5,   0,   0, 148,   5, 
      0,   0,  65, 111, 110,  57, 
    184,   1,   0,   0, 184,   1, 
      0,   0,   0,   2, 254, 255, 
    132,   1,   0,   0,  52,   0, 
      0,   0,   1,   0,  36,   0, 
      0,   0,  48,   0,   0,   0, 
     48,   0,   0,   0,  36,   0, 
      1,   0,  48,   0,   0,   0, 
      0,   0, 144,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    144,   0,   1,   2, 254, 255, 
     81,   0,   0,   5, 145,   0, 
     15, 160,   0,   0,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     81,   0,   0,   5, 146,   0, 
     15, 160, 102, 102,   6,  64, 
     51,  51, 131,  64, 205, 204, 
    204,  61, 205, 204, 140,  63, 
     81,   0,   0,   5, 147,   0, 
     15, 160, 235,  65, 191,  68, 
    102, 102,  70,  64,  51,  51, 
    163,  64,   0,   0,   0,   0, 
     31,   0,   0,   2,   5,   0, 
      0, 128,   0,   0,  15, 144, 
     31,   0,   0,   2,   5,   0, 
      1, 128,   1,   0,  15, 144, 
      1,   0,   0,   2,   0,   0, 
      1, 128, 147,   0,   0, 160, 
      4,   0,   0,   4,   0,   0, 
     15, 128,   1,   0,   0, 144, 
      0,   0,   0, 128, 146,   0, 
    228, 160,  19,   0,   0,   2, 
      1,   0,  15, 128,   0,   0, 
    228, 128,   2,   0,   0,   3, 
      0,   0,  15, 128,   0,   0, 
    228, 128,   1,   0, 228, 129, 
      4,   0,   0,   4,   1,   0, 
      3, 128,   1,   0,   0, 144, 
    147,   0,   0, 160,   1,   0, 
    228, 129,   2,   0,   0,   3, 
      1,   0,   3, 128,   1,   0, 
    228, 128, 147,   0, 233, 160, 
     46,   0,   0,   2,   0,   0, 
      1, 176,   0,   0,  85, 128, 
      9,   0,   0,   4,   2,   0, 
      1, 224,   0,   0, 228, 144, 
      0,  32, 228, 160,   0,   0, 
      0, 176,  46,   0,   0,   2, 
      0,   0,   3, 176,   1,   0, 
    225, 128,   9,   0,   0,   4, 
      1,   0,   2, 128,   0,   0, 
    228, 144,   0,  32, 228, 160, 
      0,   0,  85, 176,   9,   0, 
      0,   4,   2,   0,   2, 224, 
      0,   0, 228, 144,   0,  32, 
    228, 160,   0,   0,   0, 176, 
     46,   0,   0,   2,   0,   0, 
      1, 176,   0,   0,   0, 128, 
      9,   0,   0,   4,   1,   0, 
      1, 128,   0,   0, 228, 144, 
      0,  32, 228, 160,   0,   0, 
      0, 176,   2,   0,   0,   3, 
      0,   0,   3, 192,   1,   0, 
    228, 128, 144,   0, 228, 160, 
     46,   0,   0,   2,   0,   0, 
      3, 176,   0,   0, 238, 128, 
      1,   0,   0,   3,   1,   0, 
     15, 224,   0,  32, 228, 160, 
      0,   0,  85, 176,   1,   0, 
      0,   3,   0,   0,  15, 224, 
      0,  32, 228, 160,   0,   0, 
      0, 176,   1,   0,   0,   2, 
      0,   0,  12, 192, 145,   0, 
     68, 160, 255, 255,   0,   0, 
     83,  72,  68,  82, 248,   1, 
      0,   0,  64,   0,   1,   0, 
    126,   0,   0,   0,  89,   8, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0, 144,   0, 
      0,   0,  95,   0,   0,   3, 
    242,  16,  16,   0,   0,   0, 
      0,   0,  95,   0,   0,   3, 
     18,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
     50,  32,  16,   0,   2,   0, 
      0,   0, 103,   0,   0,   4, 
    242,  32,  16,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0,  50,   0,   0,  15, 
     50,   0,  16,   0,   0,   0, 
      0,   0,   6,  16,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0, 235,  65, 191,  68, 
    235,  65, 191,  68,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,  64,   0,   0, 205, 204, 
    204,  61, 205, 204, 140,  63, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  28,   0,   0,   5, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7, 242,  32,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   4,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
    242,  32,  16,   0,   1,   0, 
      0,   0,  70, 142,  32,   4, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     50,   0,   0,  15, 242,   0, 
     16,   0,   0,   0,   0,   0, 
      6,  16,  16,   0,   1,   0, 
      0,   0,   2,  64,   0,   0, 
    235,  65, 191,  68, 235,  65, 
    191,  68, 235,  65, 191,  68, 
    235,  65, 191,  68,   2,  64, 
      0,   0, 102, 102,   6,  64, 
    102, 102,  70,  64,  51,  51, 
    131,  64,  51,  51, 163,  64, 
     28,   0,   0,   5, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   9, 
     18,  32,  16,   0,   2,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   4,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   9, 
     34,  32,  16,   0,   2,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   4,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   9, 
     18,  32,  16,   0,   3,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   4,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   9, 
     34,  32,  16,   0,   3,   0, 
      0,   0,  70,  30,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   4,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   8, 
    194,  32,  16,   0,   3,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 128,  63,  62,   0, 
      0,   1,  83,  84,  65,  84, 
    116,   0,   0,   0,  12,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     82,  68,  69,  70, 200,   0, 
      0,   0,   1,   0,   0,   0, 
     72,   0,   0,   0,   1,   0, 
      0,   0,  28,   0,   0,   0, 
      0,   4, 254, 255,   0, 145, 
      0,   0, 148,   0,   0,   0, 
     60,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,  36,  71, 108, 111, 
     98,  97, 108, 115,   0, 171, 
    171, 171,  60,   0,   0,   0, 
      1,   0,   0,   0,  96,   0, 
      0,   0,   0,   9,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 120,   0,   0,   0, 
      0,   0,   0,   0,   0,   9, 
      0,   0,   2,   0,   0,   0, 
    132,   0,   0,   0,   0,   0, 
      0,   0, 118, 102, 117, 110, 
    105, 102, 111, 114, 109, 115, 
      0, 171,   1,   0,   3,   0, 
      1,   0,   4,   0, 144,   0, 
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
     71,  78,  72,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0,  65,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   1,   1, 
      0,   0,  80,  79,  83,  73, 
     84,  73,  79,  78,   0,  67, 
     79,  76,  79,  82,   0, 171, 
     79,  83,  71,  78, 128,   0, 
      0,   0,   4,   0,   0,   0, 
      8,   0,   0,   0, 104,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0, 104,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
     15,   0,   0,   0, 104,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
      3,  12,   0,   0, 113,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   3,   0,   0,   0, 
     15,   0,   0,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0,  83,  86,  95,  80, 111, 
    115, 105, 116, 105, 111, 110, 
      0, 171, 171, 171
};
