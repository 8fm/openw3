#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Buffer Definitions: 
//
// cbuffer Constants
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
// sampler_tex                       sampler      NA          NA    0        1
// tex                               texture  float4          2d    0        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
Opaque Custom Data - XBOX Precompiled Shader Header
// First Precompiled Shader at offset:[101]
// Embedded Data:
//  0x00000065 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x00000060 - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[5], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v0.xy
dcl_output o0.xyzw
dcl_temps 2
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v0.xyxx, t0.xyzw, s0
dp4 r1.x, r0.xyzw, cb0[1].xyzw
dp4 r1.y, r0.xyzw, cb0[2].xyzw
dp4 r1.z, r0.xyzw, cb0[3].xyzw
dp4 r1.w, r0.xyzw, cb0[4].xyzw
add r0.x, r0.w, cb0[0].w
mad r0.xyzw, cb0[0].xyzw, r0.xxxx, r1.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[101], bundle is:[163] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGCMatrixAcMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  3, param0, paramSlot 0, DefaultVal={0,0,0,0}

codeLenInByte        = 184;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 3;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_SAMPLER, 0, offset 8:11 dwords
;  extUserElements 1[2] = IMM_CONST_BUFFER, 0, offset 12:15 dwords
NumVgprs             = 8;
NumSgprs             = 22;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000001
; constBufUsage           = 0x00000001

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000001
SPIC:NUM_INTERP             = 1
SPIC:PARAM_GEN              = 0
SPIC:FOG_ADDR               = 0
SPIC:BC_OPTIMIZE_DISABLE    = 0
SPIC:PASS_FOG_THROUGH_PS    = 0
; SPI_PS_INPUT_ADDR       = 0x00000002
SPIA:PERSP_CENTER_ENA       = 1
; DB_SHADER_CONTROL       = 0x00000010
DB:Z_ORDER                  = 1
DB:CONSERVATIVE_Z_EXPORT    = 0; EXPORT_ANY_Z
; CB_SHADER_MASK          = 0x0000000F
CB:OUTPUT0_ENABLE           = 15

// Shader Instructions:
shader main
  asic(CI)
  type(PS)
                                                            // s_ps_state in s0
  s_mov_b64     s[20:21], exec                          // 000000000000: BE94047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx8  s[12:19], s[0:1], 0x08                // 000000000010: C0C60108
  v_interp_p1_f32  v2, v0, attr0.x                      // 000000000014: C8080000
  v_interp_p1_f32  v3, v0, attr0.y                      // 000000000018: C80C0100
  v_interp_p2_f32  v2, v1, attr0.x                      // 00000000001C: C8090001
  v_interp_p2_f32  v3, v1, attr0.y                      // 000000000020: C80D0101
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[0:3], v[2:3], s[4:11], s[12:15] dmask:0xf // 000000000028: F0800F00 00610002
  s_buffer_load_dwordx8  s[0:7], s[16:19], 0x04         // 000000000030: C2C01104
  s_buffer_load_dwordx8  s[8:15], s[16:19], 0x0c        // 000000000034: C2C4110C
  s_buffer_load_dwordx4  s[16:19], s[16:19], 0x00       // 000000000038: C2881100
  s_waitcnt     vmcnt(0) & lgkmcnt(0)                   // 00000000003C: BF8C0070
  v_mul_legacy_f32  v4, s3, v3                          // 000000000040: 0E080603
  v_mul_legacy_f32  v5, s7, v3                          // 000000000044: 0E0A0607
  v_mul_legacy_f32  v6, s11, v3                         // 000000000048: 0E0C060B
  v_mul_legacy_f32  v7, s15, v3                         // 00000000004C: 0E0E060F
  v_mac_legacy_f32  v4, s2, v2                          // 000000000050: 0C080402
  v_mac_legacy_f32  v5, s6, v2                          // 000000000054: 0C0A0406
  v_mac_legacy_f32  v6, s10, v2                         // 000000000058: 0C0C040A
  v_mac_legacy_f32  v7, s14, v2                         // 00000000005C: 0C0E040E
  v_mac_legacy_f32  v4, s1, v1                          // 000000000060: 0C080201
  v_mac_legacy_f32  v5, s5, v1                          // 000000000064: 0C0A0205
  v_mac_legacy_f32  v6, s9, v1                          // 000000000068: 0C0C0209
  v_mac_legacy_f32  v7, s13, v1                         // 00000000006C: 0C0E020D
  v_mac_legacy_f32  v4, s0, v0                          // 000000000070: 0C080000
  v_mac_legacy_f32  v5, s4, v0                          // 000000000074: 0C0A0004
  v_mac_legacy_f32  v6, s8, v0                          // 000000000078: 0C0C0008
  v_mac_legacy_f32  v7, s12, v0                         // 00000000007C: 0C0E000C
  v_add_f32     v0, s19, v3                             // 000000000080: 06000613
  v_mac_legacy_f32  v4, s16, v0                         // 000000000084: 0C080010
  v_mac_legacy_f32  v5, s17, v0                         // 000000000088: 0C0A0011
  v_mac_legacy_f32  v6, s18, v0                         // 00000000008C: 0C0C0012
  v_mac_legacy_f32  v7, s19, v0                         // 000000000090: 0C0E0013
  v_mul_legacy_f32  v0, v4, v7                          // 000000000094: 0E000F04
  v_mul_legacy_f32  v1, v5, v7                          // 000000000098: 0E020F05
  v_mul_legacy_f32  v2, v6, v7                          // 00000000009C: 0E040F06
  s_mov_b64     exec, s[20:21]                          // 0000000000A0: BEFE0414
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 0000000000A4: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v7                       // 0000000000A8: 5E020F02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 0000000000AC: F800140F 00000100
  s_endpgm                                              // 0000000000B4: BF810000
end


// Approximately 10 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGCMatrixAcMul[] =
{
     68,  88,  66,  67,  60, 135, 
     89, 174, 242, 231, 183, 156, 
      5, 128,  76, 166,  32,  17, 
     39, 239,   1,   0,   0,   0, 
     52,   7,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
      0,   2,   0,   0,  52,   2, 
      0,   0, 104,   2,   0,   0, 
    152,   6,   0,   0,  82,  68, 
     69,  70, 196,   1,   0,   0, 
      1,   0,   0,   0, 184,   0, 
      0,   0,   3,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    136,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    156,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 168,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 172,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0, 171, 171, 172,   0, 
      0,   0,   2,   0,   0,   0, 
    208,   0,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  32,   1, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  48,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  84,   1,   0,   0, 
     16,   0,   0,   0,  64,   0, 
      0,   0,   2,   0,   0,   0, 
    100,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     99, 120,  97, 100, 100,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     38,   1,   0,   0,  99, 120, 
    109, 117, 108,   0, 102, 108, 
    111,  97, 116,  52, 120,  52, 
      0, 171,   3,   0,   3,   0, 
      4,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  90,   1, 
      0,   0,  77, 105,  99, 114, 
    111, 115, 111, 102, 116,  32, 
     40,  82,  41,  32,  72,  76, 
     83,  76,  32,  83, 104,  97, 
    100, 101, 114,  32,  67, 111, 
    109, 112, 105, 108, 101, 114, 
     32,  70, 111, 114,  32,  68, 
    117, 114,  97, 110, 103, 111, 
     32,  57,  46,  51,  48,  46, 
     49,  50,  48,  57,  56,  46, 
     48,   0,  73,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   3,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0, 171, 171, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88,  40,   4,   0,   0, 
     80,   0,   0,   0,  10,   1, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 101,   0, 
      0,   0,  80,   0,   0,   0, 
     96,   0,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      0,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     17,   0,   0,   8,  66,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  17,   0,   0,   8, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   8,  18,   0,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     58, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     50,   0,   0,  10, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  56,   0, 
      0,   7, 114,  32,  16,   0, 
      0,   0,   0,   0, 246,  15, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   5, 
    130,  32,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    165,   0,   0,   0,  21,   0, 
      1,   0,  41, 145,  10,   0, 
    120,   1, 237,  86, 191, 107, 
     20,  65,  20, 126, 251, 227, 
     54, 123, 155, 201, 108, 206, 
    132, 160,  18, 236,  68,  35, 
    114,  24, 181,  81, 155, 136, 
     65,  45,  76,  99,  82, 133, 
     37, 220,  94, 110,  47,   9, 
    110, 126, 176, 119, 202,  34, 
    114,  70, 171,  20,  22,  10, 
     22, 254,  25,  22,  22, 169, 
     46, 199,  17, 228,  74,  17, 
     27,  27, 255,   2,  11,  33, 
    141, 133,  68, 223, 119,  55, 
     19, 151,  16, 187, 128, 130, 
    247, 193, 247, 222, 124, 239, 
    237, 188, 153, 221,  89, 246, 
    237,  43, 143, 186, 216, 221, 
    251, 248,  14,  62, 253, 188, 
    119,  99, 211,  32, 202, 241, 
    152,  93, 215,   3, 169,  11, 
     75, 180, 145, 135,  37,  58, 
     15, 131,  11,  76,  34, 164, 
    198, 152, 219, 204,  41,  38, 
    114, 154,  61,  76, 181, 158, 
    179,  29, 103,  98, 185,  49, 
    158, 115, 151, 253,   5,  38, 
     15, 233,  12,  83,  48,  81, 
     14, 208,  30, 176,  96,  24, 
     54, 204,  33, 156, 134,  81, 
     64,  94, 239,  21,  53, 179, 
     53, 232,  44, 204, 209, 184, 
    200,  84, 183, 118,  36, 176, 
     55,  93,  11,  30, 251,   4, 
    112, 191, 127,  27, 189, 125, 
    253, 182, 160,  62,  39,   0, 
    231, 145, 133, 100, 226,  26, 
    224, 146,  58,  71, 253, 124, 
    167, 148, 198, 116, 220,  91, 
    139, 249,  39, 252, 100,  12, 
     99, 144, 129, 214, 168, 239, 
     28,  84, 253,  63, 161, 223, 
     85,  60,  11, 188, 139, 120, 
     54,  62,  83,   3, 249,   9, 
     38, 209, 230,  14, 248, 242, 
     36, 125, 135, 210, 218,  56, 
    164,  45, 104,  20, 203, 234, 
      1, 150,  22,  45, 152, 190, 
    185, 224, 143, 171, 188,  69, 
      1, 235,  32, 163, 231,  89, 
    207, 103, 116, 137, 117,  41, 
    163, 203, 172, 203,   7,  90, 
    213, 247, 187, 235, 153,   3, 
     56, 103,  14,  54, 254, 101, 
    246, 247, 120,  60, 236, 239, 
    241, 120, 216, 223, 227, 177, 
    177, 139, 134, 253, 186, 217, 
    240, 246, 155, 166, 245, 163, 
     73,  70, 187, 229,  26, 239, 
    185,  53, 185,  29,  50,  68, 
    199, 160, 124, 199,  48, 134, 
     58,  79, 233, 197,  14, 249, 
    155, 223,  76,  10, 201,  46, 
    180, 218, 162, 176, 219, 166, 
    194,  86, 123, 131, 227, 150, 
    227, 202,   1, 199, 147, 131, 
    142, 144, 190,  35, 165, 105, 
    187, 194, 177,  61, 225, 217, 
     66,  72,  91,  10, 195, 116, 
     69, 206, 244,  68, 222,  20, 
     98, 200, 148, 220, 222,  93, 
     97, 147,  39,  92,  18,  66, 
    144,  20,  35,  14,  57, 195, 
     28,  43, 112, 236,   4, 199, 
     70,  56, 102, 251,  36, 115, 
    190,  41,  29, 223, 150, 163, 
    246, 126, 147, 191, 163, 189, 
    239, 240, 168, 254, 142,  62, 
    219,  57, 197, 182,  59, 100, 
    160,   7, 100, 193,  61, 214, 
    208,  57, 244,  91, 238,   7, 
    134, 190,   6, 255,  23, 208, 
    111,  84,  63, 222,  82,  30, 
     61, 132, 227, 220, 195, 245, 
    204,  30, 182,  85, 254, 173, 
    242, 168,  55, 200,  20,  75, 
     19, 231, 226, 133, 107, 249, 
     79,  42, 254,  65, 121, 212, 
    192,  63,  88, 171, 249, 228, 
    203,  61,  12,  24,  95,  85, 
    174, 196, 196, 252, 234, 245, 
    160,  28, 198, 225, 227,  90, 
    177, 190, 158, 172,  63,  40, 
    214, 234,  73,  20, 174,  94, 
     14,  42, 209, 163,  32,  74, 
    235,  81, 178,  22, 198, 193, 
     82,  53, 189,  26, 204,  38, 
    139, 193, 253, 104, 173,  18, 
     37, 193, 244, 149, 233, 201, 
     52, 152,  93,  14,  89, 212, 
    130, 219, 115,  81,  58, 119, 
    231, 214,  76,  88,  79,  86, 
    210, 155, 139,  51,  15, 227, 
     98,  53, 158, 156,  44,  46, 
    199, 181, 152,  87, 232, 161, 
    162, 214, 199, 127,  27, 218, 
    217, 106, 184, 178,  70, 191, 
      0, 218, 191,  37,  45,   0, 
      0,   0,  83,  84,  65,  84, 
    148,   0,   0,   0,  10,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
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
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0
};
