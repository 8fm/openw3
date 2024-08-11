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
//   float4x4 cxmul;                    // Offset:   16 Size:    64
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// tex                               sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
// $Globals                          cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float      w
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
//
// Constant buffer to DX9 shader constant mappings:
//
// Target Reg Buffer  Start Reg # of Regs        Data Conversion
// ---------- ------- --------- --------- ----------------------
// c0         cb0             0         5  ( FLT, FLT, FLT, FLT)
//
//
// Sampler/Resource to DX9 shader sampler mappings:
//
// Target Sampler Source Sampler  Source Resource
// -------------- --------------- ----------------
// s0             s0              t0               
//
//
// Level9 shader bytecode:
//
    ps_2_x
    dcl t0
    dcl_pp t1.xy
    dcl_2d s0
    texld r0, t1, s0
    dp4 r1.x, r0, c1
    dp4 r1.y, r0, c2
    dp4 r1.z, r0, c3
    dp4 r1.w, r0, c4
    add r0.x, r0.w, c0.w
    mad r0, c0, r0.x, r1
    mul r1.w, r0.w, t0.w
    mul r1.xyz, r0, r1.w
    mov oC0, r1

// approximately 10 instruction slots used (1 texture, 9 arithmetic)
ps_4_0
dcl_constantbuffer cb0[5], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.w
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v1.xyxx, t0.xyzw, s0
dp4 r1.x, r0.xyzw, cb0[1].xyzw
dp4 r1.y, r0.xyzw, cb0[2].xyzw
dp4 r1.z, r0.xyzw, cb0[3].xyzw
dp4 r1.w, r0.xyzw, cb0[4].xyzw
add r0.x, r0.w, cb0[0].w
mad r0.xyzw, cb0[0].xyzw, r0.xxxx, r1.xyzw
mul r0.w, r0.w, v0.w
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
// Approximately 11 instruction slots used
#endif

const BYTE pBinary_D3D1xFL93_FTexTGCMatrixAcEAlphaMul[] =
{
     68,  88,  66,  67, 166, 234, 
    253,  60,  58,  33, 207,  12, 
    215, 192, 109, 201,  64, 164, 
     65, 103,   1,   0,   0,   0, 
     36,   5,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
     64,   1,   0,   0, 228,   2, 
      0,   0,  96,   3,   0,   0, 
    160,   4,   0,   0, 240,   4, 
      0,   0,  65, 111, 110,  57, 
      0,   1,   0,   0,   0,   1, 
      0,   0,   0,   2, 255, 255, 
    204,   0,   0,   0,  52,   0, 
      0,   0,   1,   0,  40,   0, 
      0,   0,  52,   0,   0,   0, 
     52,   0,   1,   0,  36,   0, 
      0,   0,  52,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   0,   0, 
      0,   0,   1,   2, 255, 255, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   1,   0,  35, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     66,   0,   0,   3,   0,   0, 
     15, 128,   1,   0, 228, 176, 
      0,   8, 228, 160,   9,   0, 
      0,   3,   1,   0,   1, 128, 
      0,   0, 228, 128,   1,   0, 
    228, 160,   9,   0,   0,   3, 
      1,   0,   2, 128,   0,   0, 
    228, 128,   2,   0, 228, 160, 
      9,   0,   0,   3,   1,   0, 
      4, 128,   0,   0, 228, 128, 
      3,   0, 228, 160,   9,   0, 
      0,   3,   1,   0,   8, 128, 
      0,   0, 228, 128,   4,   0, 
    228, 160,   2,   0,   0,   3, 
      0,   0,   1, 128,   0,   0, 
    255, 128,   0,   0, 255, 160, 
      4,   0,   0,   4,   0,   0, 
     15, 128,   0,   0, 228, 160, 
      0,   0,   0, 128,   1,   0, 
    228, 128,   5,   0,   0,   3, 
      1,   0,   8, 128,   0,   0, 
    255, 128,   0,   0, 255, 176, 
      5,   0,   0,   3,   1,   0, 
      7, 128,   0,   0, 228, 128, 
      1,   0, 255, 128,   1,   0, 
      0,   2,   0,   8,  15, 128, 
      1,   0, 228, 128, 255, 255, 
      0,   0,  83,  72,  68,  82, 
    156,   1,   0,   0,  64,   0, 
      0,   0, 103,   0,   0,   0, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
    130,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   1,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      2,   0,   0,   0,  69,   0, 
      0,   9, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  17,   0, 
      0,   8,  18,   0,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     17,   0,   0,   8,  34,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  17,   0,   0,   8, 
     66,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  17,   0, 
      0,   8, 130,   0,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   8,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  58, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  50,   0,   0,  10, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     56,   0,   0,   7, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  58,  16,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  83,  84,  65,  84, 
    116,   0,   0,   0,  11,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   7,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     82,  68,  69,  70,  56,   1, 
      0,   0,   1,   0,   0,   0, 
    140,   0,   0,   0,   3,   0, 
      0,   0,  28,   0,   0,   0, 
      0,   4, 255, 255,   0, 145, 
      0,   0,   4,   1,   0,   0, 
    124,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 124,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     12,   0,   0,   0, 128,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    116, 101, 120,   0,  36,  71, 
    108, 111,  98,  97, 108, 115, 
      0, 171, 171, 171, 128,   0, 
      0,   0,   2,   0,   0,   0, 
    164,   0,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 212,   0, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 220,   0,   0,   0, 
      0,   0,   0,   0, 236,   0, 
      0,   0,  16,   0,   0,   0, 
     64,   0,   0,   0,   2,   0, 
      0,   0, 244,   0,   0,   0, 
      0,   0,   0,   0,  99, 120, 
     97, 100, 100,   0, 171, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  99, 120, 
    109, 117, 108,   0, 171, 171, 
      3,   0,   3,   0,   4,   0, 
      4,   0,   0,   0,   0,   0, 
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
     72,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   8,   0,   0, 
     62,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,   3,   3,   0,   0, 
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
