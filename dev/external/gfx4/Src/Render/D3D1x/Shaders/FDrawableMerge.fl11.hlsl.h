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
//   float4x4 cxmul;                    // Offset:    0 Size:    64
//   float4x4 cxmul1;                   // Offset:   64 Size:    64
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex[0]                    sampler      NA          NA    0        1
// sampler_tex[1]                    sampler      NA          NA    1        1
// tex[0]                            texture  float4          2d    0        1
// tex[1]                            texture  float4          2d    1        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// TEXCOORD                 1     zw        0     NONE   float     zw
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
// First Precompiled Shader at offset:[131]
// Embedded Data:
//  0x00000083 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000007e - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[8], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.xy
dcl_input_ps linear v0.zw
dcl_output o0.xyzw
dcl_temps 3
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v0.xyxx, t0.xyzw, s0
dp4 r1.x, r0.xyzw, cb0[0].xyzw
dp4 r1.y, r0.xyzw, cb0[1].xyzw
dp4 r1.z, r0.xyzw, cb0[2].xyzw
dp4 r1.w, r0.xyzw, cb0[3].xyzw
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v0.zwzz, t1.xyzw, s1
dp4 r2.x, r0.xyzw, cb0[4].xyzw
dp4 r2.y, r0.xyzw, cb0[5].xyzw
dp4 r2.z, r0.xyzw, cb0[6].xyzw
dp4 r2.w, r0.xyzw, cb0[7].xyzw
add o0.xyzw, r1.xyzw, r2.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[131], bundle is:[185] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FDrawableMerge.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0}

codeLenInByte        = 284;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 5;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
;  extUserElements 1[4] = IMM_CONST_BUFFER, 0, offset 24:27 dwords
NumVgprs             = 14;
NumSgprs             = 38;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
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
  s_mov_b64     s[36:37], exec                          // 000000000000: BEA4047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx4  s[20:23], s[0:1], 0x10                // 000000000010: C08A0110
  v_interp_p1_f32  v2, v0, attr0.x                      // 000000000014: C8080000
  v_interp_p1_f32  v3, v0, attr0.y                      // 000000000018: C80C0100
  v_interp_p2_f32  v2, v1, attr0.x                      // 00000000001C: C8090001
  v_interp_p2_f32  v3, v1, attr0.y                      // 000000000020: C80D0101
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[5:8], v[2:3], s[4:11], s[20:23] dmask:0xf // 000000000028: F0800F00 00A10502
  s_load_dwordx8  s[12:19], s[0:1], 0x08                // 000000000030: C0C60108
  s_load_dwordx4  s[24:27], s[0:1], 0x14                // 000000000034: C08C0114
  v_interp_p1_f32  v9, v0, attr0.z                      // 000000000038: C8240200
  v_interp_p1_f32  v10, v0, attr0.w                     // 00000000003C: C8280300
  v_interp_p2_f32  v9, v1, attr0.z                      // 000000000040: C8250201
  v_interp_p2_f32  v10, v1, attr0.w                     // 000000000044: C8290301
  s_waitcnt     lgkmcnt(0)                              // 000000000048: BF8C007F
  image_sample  v[0:3], v[9:10], s[12:19], s[24:27] dmask:0xf // 00000000004C: F0800F00 00C30009
  s_load_dwordx4  s[0:3], s[0:1], 0x18                  // 000000000054: C0800118
  s_waitcnt     lgkmcnt(0)                              // 000000000058: BF8C007F
  s_buffer_load_dwordx8  s[4:11], s[0:3], 0x00          // 00000000005C: C2C20100
  s_buffer_load_dwordx8  s[12:19], s[0:3], 0x08         // 000000000060: C2C60108
  s_buffer_load_dwordx8  s[20:27], s[0:3], 0x10         // 000000000064: C2CA0110
  s_buffer_load_dwordx8  s[28:35], s[0:3], 0x18         // 000000000068: C2CE0118
  s_waitcnt     vmcnt(1) & lgkmcnt(0)                   // 00000000006C: BF8C0071
  v_mul_legacy_f32  v4, s7, v8                          // 000000000070: 0E081007
  v_mul_legacy_f32  v9, s11, v8                         // 000000000074: 0E12100B
  v_mul_legacy_f32  v10, s15, v8                        // 000000000078: 0E14100F
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 00000000007C: BF8C0F70
  v_mul_legacy_f32  v11, s23, v3                        // 000000000080: 0E160617
  v_mul_legacy_f32  v12, s27, v3                        // 000000000084: 0E18061B
  v_mul_legacy_f32  v13, s31, v3                        // 000000000088: 0E1A061F
  v_mul_legacy_f32  v8, s19, v8                         // 00000000008C: 0E101013
  v_mul_legacy_f32  v3, s35, v3                         // 000000000090: 0E060623
  v_mac_legacy_f32  v4, s6, v7                          // 000000000094: 0C080E06
  v_mac_legacy_f32  v9, s10, v7                         // 000000000098: 0C120E0A
  v_mac_legacy_f32  v10, s14, v7                        // 00000000009C: 0C140E0E
  v_mac_legacy_f32  v8, s18, v7                         // 0000000000A0: 0C100E12
  v_mac_legacy_f32  v11, s22, v2                        // 0000000000A4: 0C160416
  v_mac_legacy_f32  v12, s26, v2                        // 0000000000A8: 0C18041A
  v_mac_legacy_f32  v13, s30, v2                        // 0000000000AC: 0C1A041E
  v_mac_legacy_f32  v3, s34, v2                         // 0000000000B0: 0C060422
  v_mac_legacy_f32  v4, s5, v6                          // 0000000000B4: 0C080C05
  v_mac_legacy_f32  v9, s9, v6                          // 0000000000B8: 0C120C09
  v_mac_legacy_f32  v10, s13, v6                        // 0000000000BC: 0C140C0D
  v_mac_legacy_f32  v8, s17, v6                         // 0000000000C0: 0C100C11
  v_mac_legacy_f32  v11, s21, v1                        // 0000000000C4: 0C160215
  v_mac_legacy_f32  v12, s25, v1                        // 0000000000C8: 0C180219
  v_mac_legacy_f32  v13, s29, v1                        // 0000000000CC: 0C1A021D
  v_mac_legacy_f32  v3, s33, v1                         // 0000000000D0: 0C060221
  v_mac_legacy_f32  v4, s4, v5                          // 0000000000D4: 0C080A04
  v_mac_legacy_f32  v9, s8, v5                          // 0000000000D8: 0C120A08
  v_mac_legacy_f32  v10, s12, v5                        // 0000000000DC: 0C140A0C
  v_mac_legacy_f32  v8, s16, v5                         // 0000000000E0: 0C100A10
  v_mac_legacy_f32  v11, s20, v0                        // 0000000000E4: 0C160014
  v_mac_legacy_f32  v12, s24, v0                        // 0000000000E8: 0C180018
  v_mac_legacy_f32  v13, s28, v0                        // 0000000000EC: 0C1A001C
  v_mac_legacy_f32  v3, s32, v0                         // 0000000000F0: 0C060020
  v_add_f32     v0, v4, v11                             // 0000000000F4: 06001704
  v_add_f32     v1, v9, v12                             // 0000000000F8: 06021909
  v_add_f32     v2, v10, v13                            // 0000000000FC: 06041B0A
  v_add_f32     v3, v8, v3                              // 000000000100: 06060708
  s_mov_b64     exec, s[36:37]                          // 000000000104: BEFE0424
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 000000000108: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v3                       // 00000000010C: 5E020702
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000110: F800140F 00000100
  s_endpgm                                              // 000000000118: BF810000
end


// Approximately 12 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FDrawableMerge[] =
{
     68,  88,  66,  67,  16,  37, 
    212, 189, 123,  63,  55,  25, 
    251,  51,  65,  62,  38, 150, 
     42, 231,   1,   0,   0,   0, 
     76,   8,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     48,   2,   0,   0, 124,   2, 
      0,   0, 176,   2,   0,   0, 
    176,   7,   0,   0,  82,  68, 
     69,  70, 244,   1,   0,   0, 
      1,   0,   0,   0,  20,   1, 
      0,   0,   5,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    183,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    220,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 235,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 250,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
      1,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,   8,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114,  95, 
    116, 101, 120,  91,  48,  93, 
      0, 115,  97, 109, 112, 108, 
    101, 114,  95, 116, 101, 120, 
     91,  49,  93,   0, 116, 101, 
    120,  91,  48,  93,   0, 116, 
    101, 120,  91,  49,  93,   0, 
     67, 111, 110, 115, 116,  97, 
    110, 116, 115,   0, 171, 171, 
      8,   1,   0,   0,   2,   0, 
      0,   0,  44,   1,   0,   0, 
    128,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    124,   1,   0,   0,   0,   0, 
      0,   0,  64,   0,   0,   0, 
      2,   0,   0,   0, 140,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 176,   1, 
      0,   0,  64,   0,   0,   0, 
     64,   0,   0,   0,   2,   0, 
      0,   0, 140,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  99, 120, 109, 117, 
    108,   0, 102, 108, 111,  97, 
    116,  52, 120,  52,   0, 171, 
      3,   0,   3,   0,   4,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 130,   1,   0,   0, 
     99, 120, 109, 117, 108,  49, 
      0,  77, 105,  99, 114, 111, 
    115, 111, 102, 116,  32,  40, 
     82,  41,  32,  72,  76,  83, 
     76,  32,  83, 104,  97, 100, 
    101, 114,  32,  67, 111, 109, 
    112, 105, 108, 101, 114,  32, 
     70, 111, 114,  32,  68, 117, 
    114,  97, 110, 103, 111,  32, 
     57,  46,  51,  48,  46,  49, 
     50,  48,  57,  56,  46,  48, 
      0, 171,  73,  83,  71,  78, 
     68,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   3,   0,   0, 
     56,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  12,  12,   0,   0, 
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
     69,  88, 248,   4,   0,   0, 
     80,   0,   0,   0,  62,   1, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 131,   0, 
      0,   0,  80,   0,   0,   0, 
    126,   0,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
    194,  16,  16,   0,   0,   0, 
      0,   0, 101,   0,   0,   3, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 104,   0,   0,   2, 
      3,   0,   0,   0,  69,   0, 
      0, 139, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     17,   0,   0,   8,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  17,   0,   0,   8, 
     34,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   8,  66,   0,  16,   0, 
      1,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     17,   0,   0,   8, 130,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      0,   0,   0,   0, 230,  26, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,  17,   0, 
      0,   8,  18,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     17,   0,   0,   8,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70, 142,  32,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,  17,   0,   0,   8, 
     66,   0,  16,   0,   2,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,  17,   0, 
      0,   8, 130,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   7,   0,   0,   0, 
      0,   0,   0,   7, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      2,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    187,   0,   0,   0,  21,   0, 
      1,   0,  41,  17,  11,   0, 
    120,   1, 237,  86, 207,  79, 
    212,  64,  20, 126, 211, 150, 
    217, 110, 121, 116, 203,  82, 
    151, 178, 172, 226,  15,  48, 
    120, 217,   4, 229, 164,  30, 
    208,  16,  99,  98, 188, 136, 
      7,  66,  26,  66,  87,  10, 
     24, 151,  31, 238,  18,  93, 
     57, 192, 114,  53,  28,  56, 
    120, 209, 152,  16,  19, 254, 
     13, 119,  55, 171,  49,  27, 
     99, 252,  43, 188, 235, 193, 
      3,   7, 131, 190, 215, 109, 
    177,  33, 114,  35, 209,  68, 
    190, 228, 123, 111, 190, 121, 
     51, 111,  94, 103, 154, 118, 
    186,  59,  33,  64, 237, 125, 
    115, 135, 253, 237, 129, 103, 
    213,  61,   1, 208,  65, 109, 
    114, 129, 103,  84, 116, 182, 
      0, 223, 147, 108,   1, 134, 
    217, 240,   0,   5, 192,  36, 
    119, 145, 152,  35,  61,  70, 
    158,  99,  17, 219,  24, 107, 
    188,  36,  59,  72,  76,  19, 
    215, 104, 206,  29, 242,  55, 
    136,  42,  49,  79, 236,  33, 
    114,  58,   6, 133,  15,  32, 
    217,  16,  52,  54, 135, 192, 
    107,  70, 224, 242, 194, 210, 
    130, 156, 241,  28,  48, 196, 
    230, 207,  24,  39, 114, 253, 
     71, 129, 107, 139, 114, 113, 
    125,  92,  39,  35, 190, 246, 
    223,  66, 123, 191, 218, 150, 
    159, 153,  91, 209,  57,  49, 
    248,  60, 226, 224, 231, 140, 
    186,  94, 132, 155,  21, 157, 
    239, 110, 168, 121,  12,  63, 
     91, 131, 120,  20, 126,  18, 
    114, 220, 136,  33, 210, 156, 
     95,  66, 138, 236, 255, 139, 
    232,  93, 229, 189, 224, 119, 
    199,  34, 198, 119, 132, 227, 
     55, 137,   0, 213,  58, 115, 
    219, 129,  61,  86, 145,  22, 
    135, 180, 202, 154, 147, 197, 
     53,  31, 184,  10, 211,  74, 
     66, 153,  78, 229, 194, 184, 
     10,  46, 105,  55, 166, 167, 
     72,  79, 197, 244,  12, 233, 
    153, 152,  46, 144,  46,  28, 
    232,  48, 127,  42,  88,  79, 
     81, 249, 156, 169, 115, 253, 
     95, 230,  73, 141, 199, 195, 
    147,  26, 143, 135,  39,  53, 
     30,  27,   3, 172, 107, 187, 
    181, 117,  99, 191, 166, 168, 
     63, 106,  32, 154,  13,  75, 
     60, 167,  95, 147, 222,   2, 
    129,  45,   1, 201, 150,  16, 
     93, 173,  13, 216, 170,  67, 
    170, 250,  77, 233, 120,   3, 
    186, 248, 208, 176, 197,  86, 
      3, 148, 193,  22, 168, 195, 
     45, 161,  12, 181, 132, 122, 
    233,  96,  76,  18, 222, 129, 
     35, 170, 141,  64, 139, 102, 
    147, 198,  55,  45, 241, 177, 
    233, 136, 207, 205, 199, 212, 
    151, 176, 116, 179, 211,  74, 
    155,  41, 203,  54,  87,  82, 
     91, 245,  94, 153,  49, 251, 
    165,  99,  14, 200, 172, 217, 
     99,  89, 230,   5,  41,  77, 
    105, 234, 104, 152, 105,  52, 
     77,  27, 211, 166, 133,  25, 
     45, 131,  89, 205, 193,  51, 
     90,  22, 207, 107,  18,  59, 
     80, 199,  36, 166, 177,  11, 
    109, 236,  70,  11,  79,  41, 
     25, 236,  83,  28,  60, 173, 
    100, 241, 156,  34,  81,  51, 
    116, 212, 141,  52, 162,  97, 
    163, 101,  88, 104,  67,   6, 
     29, 112,  48,   7,  89,  60, 
     11,  20, 239,   5, 153, 236, 
     83, 164, 209, 175,  73,  61, 
     33, 229, 160, 182,  95, 163, 
    111, 117, 251,  91, 111,  71, 
    223, 234, 205, 122,  31, 217, 
    160,  73, 136, 238,  40,  17, 
    232,  26,  16, 132, 216, 240, 
    149, 128, 117, 208,  65, 224, 
    255,  17, 253, 131,   2, 205, 
    180, 137, 172, 163,  28,  14, 
    145, 117, 206, 160,   6,   1, 
     67,  31, 206, 163,  59,   2, 
    207, 250, 141, 235,  97, 124, 
     52, 244, 188,  30,  95,  43, 
    157, 175, 111, 119,  94, 111, 
     95, 219, 156,  12, 251, 239, 
    135, 158, 115, 112, 115, 242, 
    213, 153, 141,  79,  97, 223, 
     74, 232,  93,  34, 207, 159, 
    187, 234,  22, 188, 162, 183, 
     86, 206, 175,  46, 151, 150, 
     31, 229, 203, 171,  37, 223, 
     91, 188, 236, 206, 250,  79, 
     92, 191, 178, 234, 151, 150, 
    188, 162,  59,  63,  87,  25, 
    117,  39,  74,  15, 220, 123, 
    254, 210, 172,  95, 114, 199, 
    175, 140, 143,  84, 220, 137, 
      5, 143,  68, 217, 189,  53, 
     94, 242, 158, 122, 133, 162, 
    127, 215,  47, 205, 251, 249, 
    185, 226, 200,  72, 126, 161, 
     88,  46,  82, 246,  54, 190, 
    132, 107, 242,  29,  40,  65, 
     92, 244,  30,  46, 193,  47, 
    180,   9,  57,  13,   0,   0, 
     83,  84,  65,  84, 148,   0, 
      0,   0,  12,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      9,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
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
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
