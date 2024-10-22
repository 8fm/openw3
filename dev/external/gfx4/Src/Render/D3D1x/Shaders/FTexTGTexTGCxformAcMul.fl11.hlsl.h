#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler For Durango 9.30.12098.0
//
//
///
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_tex[0]                    sampler      NA          NA    0        1
// sampler_tex[1]                    sampler      NA          NA    1        1
// tex[0]                            texture  float4          2d    0        1
// tex[1]                            texture  float4          2d    1        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COLOR                    0   xyzw        0     NONE   float   x   
// TEXCOORD                 0   xyzw        1     NONE   float   xyzw
// TEXCOORD                 1   xyzw        2     NONE   float   xyzw
// TEXCOORD                 2   xy          3     NONE   float   xy  
// TEXCOORD                 3     zw        3     NONE   float     zw
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
// First Precompiled Shader at offset:[127]
// Embedded Data:
//  0x0000007f - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x0000007a - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_input_ps linear v0.x
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xyzw
dcl_input_ps linear v3.xy
dcl_input_ps linear v3.zw
dcl_output o0.xyzw
dcl_temps 2
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, v3.xyxx, t0.xyzw, s0
sample_indexable(texture2d)(float,float,float,float) r1.xyzw, v3.zwzz, t1.xyzw, s1
add r0.xyzw, r0.xyzw, -r1.xyzw
mad r0.xyzw, v0.xxxx, r0.xyzw, r1.xyzw
mov r1.xyz, v2.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v2.wwww
mad r0.xyzw, v1.xyzw, r0.wwww, r0.xyzw
mul o0.xyz, r0.wwww, r0.xyzx
mov o0.w, r0.w
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[127], bundle is:[184] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FTexTGTexTGCxformAcMul.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask  1, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask 15, param2, paramSlot 2, DefaultVal={0,0,0,0};   [3] generic, usageIdx  3, channelMask 15, param3, paramSlot 3, DefaultVal={0,0,0,0}

codeLenInByte        = 268;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 4;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_SAMPLER, 0, offset 16:19 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 1, offset 20:23 dwords
NumVgprs             = 14;
NumSgprs             = 30;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000003
; constBufUsage           = 0x00000000

; SPI_SHADER_PGM_RSRC2_PS = 0x00000004
SSPRP:SCRATCH_EN            = 0
SSPRP:USER_SGPR             = 2
SSPRP:TRAP_PRESENT          = 0
SSPRP:WAVE_CNT_EN           = 0
SSPRP:EXTRA_LDS_SIZE        = 0
SSPRP:EXCP_EN               = 0
; SPI_SHADER_Z_FORMAT     = 0x00000000
SPZF:Z_EXPORT_FORMAT        = 0; SPI_SHADER_ZERO
; SPI_PS_IN_CONTROL       = 0x00000004
SPIC:NUM_INTERP             = 4
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
  s_mov_b64     s[28:29], exec                          // 000000000000: BE9C047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x00                 // 00000000000C: C0C20100
  s_load_dwordx8  s[20:27], s[0:1], 0x10                // 000000000010: C0CA0110
  v_interp_p1_f32  v2, v0, attr3.x                      // 000000000014: C8080C00
  v_interp_p1_f32  v3, v0, attr3.y                      // 000000000018: C80C0D00
  v_interp_p2_f32  v2, v1, attr3.x                      // 00000000001C: C8090C01
  v_interp_p2_f32  v3, v1, attr3.y                      // 000000000020: C80D0D01
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  image_sample  v[6:9], v[2:3], s[4:11], s[20:23] dmask:0xf // 000000000028: F0800F00 00A10602
  s_load_dwordx8  s[12:19], s[0:1], 0x08                // 000000000030: C0C60108
  v_interp_p1_f32  v4, v0, attr3.z                      // 000000000034: C8100E00
  v_interp_p1_f32  v5, v0, attr3.w                      // 000000000038: C8140F00
  v_interp_p2_f32  v4, v1, attr3.z                      // 00000000003C: C8110E01
  v_interp_p2_f32  v5, v1, attr3.w                      // 000000000040: C8150F01
  s_waitcnt     lgkmcnt(0)                              // 000000000044: BF8C007F
  image_sample  v[2:5], v[4:5], s[12:19], s[24:27] dmask:0xf // 000000000048: F0800F00 00C30204
  v_interp_p1_f32  v10, v0, attr0.x                     // 000000000050: C8280000
  v_interp_p2_f32  v10, v1, attr0.x                     // 000000000054: C8290001
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000058: BF8C0F70
  v_sub_f32     v6, v6, v2                              // 00000000005C: 080C0506
  v_sub_f32     v7, v7, v3                              // 000000000060: 080E0707
  v_sub_f32     v8, v8, v4                              // 000000000064: 08100908
  v_interp_p1_f32  v11, v0, attr2.x                     // 000000000068: C82C0800
  v_interp_p1_f32  v12, v0, attr2.y                     // 00000000006C: C8300900
  v_interp_p1_f32  v13, v0, attr2.z                     // 000000000070: C8340A00
  v_sub_f32     v9, v9, v5                              // 000000000074: 08120B09
  v_mac_legacy_f32  v2, v10, v6                         // 000000000078: 0C040D0A
  v_mac_legacy_f32  v3, v10, v7                         // 00000000007C: 0C060F0A
  v_mac_legacy_f32  v4, v10, v8                         // 000000000080: 0C08110A
  v_interp_p1_f32  v6, v0, attr2.w                      // 000000000084: C8180B00
  v_mac_legacy_f32  v5, v10, v9                         // 000000000088: 0C0A130A
  v_interp_p2_f32  v11, v1, attr2.x                     // 00000000008C: C82D0801
  v_interp_p2_f32  v12, v1, attr2.y                     // 000000000090: C8310901
  v_interp_p2_f32  v13, v1, attr2.z                     // 000000000094: C8350A01
  v_interp_p1_f32  v7, v0, attr1.x                      // 000000000098: C81C0400
  v_interp_p1_f32  v8, v0, attr1.y                      // 00000000009C: C8200500
  v_interp_p1_f32  v9, v0, attr1.z                      // 0000000000A0: C8240600
  v_interp_p2_f32  v6, v1, attr2.w                      // 0000000000A4: C8190B01
  v_interp_p1_f32  v0, v0, attr1.w                      // 0000000000A8: C8000700
  v_mul_legacy_f32  v2, v2, v11                         // 0000000000AC: 0E041702
  v_mul_legacy_f32  v3, v3, v12                         // 0000000000B0: 0E061903
  v_mul_legacy_f32  v4, v4, v13                         // 0000000000B4: 0E081B04
  v_interp_p2_f32  v7, v1, attr1.x                      // 0000000000B8: C81D0401
  v_interp_p2_f32  v8, v1, attr1.y                      // 0000000000BC: C8210501
  v_interp_p2_f32  v9, v1, attr1.z                      // 0000000000C0: C8250601
  v_mul_legacy_f32  v2, v2, v6                          // 0000000000C4: 0E040D02
  v_mul_legacy_f32  v3, v3, v6                          // 0000000000C8: 0E060D03
  v_mul_legacy_f32  v4, v4, v6                          // 0000000000CC: 0E080D04
  v_mul_legacy_f32  v5, v5, v6                          // 0000000000D0: 0E0A0D05
  v_interp_p2_f32  v0, v1, attr1.w                      // 0000000000D4: C8010701
  v_mac_legacy_f32  v2, v7, v5                          // 0000000000D8: 0C040B07
  v_mac_legacy_f32  v3, v8, v5                          // 0000000000DC: 0C060B08
  v_mac_legacy_f32  v4, v9, v5                          // 0000000000E0: 0C080B09
  v_mac_legacy_f32  v5, v0, v5                          // 0000000000E4: 0C0A0B00
  v_mul_legacy_f32  v0, v2, v5                          // 0000000000E8: 0E000B02
  v_mul_legacy_f32  v1, v3, v5                          // 0000000000EC: 0E020B03
  v_mul_legacy_f32  v2, v4, v5                          // 0000000000F0: 0E040B04
  s_mov_b64     exec, s[28:29]                          // 0000000000F4: BEFE041C
  v_cvt_pkrtz_f16_f32  v0, v0, v1                       // 0000000000F8: 5E000300
  v_cvt_pkrtz_f16_f32  v1, v2, v5                       // 0000000000FC: 5E020B02
  exp           mrt0, v0, v0, v1, v1 compr vm           // 000000000100: F800140F 00000100
  s_endpgm                                              // 000000000108: BF810000
end


// Approximately 12 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FTexTGTexTGCxformAcMul[] =
{
     68,  88,  66,  67,  15,  52, 
    145,  23, 139,  31, 114,  73, 
      4, 189, 236,  45, 119, 161, 
     65,  99,   1,   0,   0,   0, 
    180,   7,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     96,   1,   0,   0, 248,   1, 
      0,   0,  44,   2,   0,   0, 
     24,   7,   0,   0,  82,  68, 
     69,  70,  36,   1,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    232,   0,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    188,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 203,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 218,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    225,   0,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   1,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 116, 101, 
    120,  91,  48,  93,   0, 115, 
     97, 109, 112, 108, 101, 114, 
     95, 116, 101, 120,  91,  49, 
     93,   0, 116, 101, 120,  91, 
     48,  93,   0, 116, 101, 120, 
     91,  49,  93,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  70, 111, 114, 
     32,  68, 117, 114,  97, 110, 
    103, 111,  32,  57,  46,  51, 
     48,  46,  49,  50,  48,  57, 
     56,  46,  48,   0,  73,  83, 
     71,  78, 144,   0,   0,   0, 
      5,   0,   0,   0,   8,   0, 
      0,   0, 128,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   1, 
      0,   0, 134,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0, 134,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,  15,  15, 
      0,   0, 134,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   3,   3, 
      0,   0, 134,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,  12,  12, 
      0,   0,  67,  79,  76,  79, 
     82,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  97, 114, 103, 101, 
    116,   0, 171, 171,  83,  72, 
     69,  88, 228,   4,   0,   0, 
     80,   0,   0,   0,  57,   1, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 127,   0, 
      0,   0,  80,   0,   0,   0, 
    122,   0,   0,   0, 106,   8, 
      0,   1,  90,   0,   0,   3, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  90,   0,   0,   3, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   1,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3,  18,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      2,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      3,   0,   0,   0,  98,  16, 
      0,   3, 194,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      3,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      1,   0,   0,   0, 230,  26, 
     16,   0,   3,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   8, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
      6,  16,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 114,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  18,  16,   0,   2,   0, 
      0,   0,  54,   0,   0,   5, 
    130,   0,  16,   0,   1,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0, 246,  31, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  30,  16,   0,   1,   0, 
      0,   0, 246,  15,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,  32, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 130,  32,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     62,   0,   0,   1,  53,  16, 
      0,   0, 186,   0,   0,   0, 
     21,   0,   1,   0,  41, 249, 
     10,   0, 120,   1, 237,  86, 
    207, 107,  19,  65,  20, 126, 
    179, 187, 217, 157, 253, 157, 
    214,  20,  91,  27, 181, 130, 
    133,  10,  26,  77, 173,  32, 
    130,  90, 177, 232,  73,  40, 
    182, 167, 178, 148, 166, 109, 
     98, 197,  52, 149, 164,  74, 
    240,  80,  43, 232, 173,   7, 
     15, 189,   8, 130, 120, 244, 
     46, 120,  75, 130, 130, 140, 
     82,   4, 193,  63, 162,   7, 
    133,  30,  21,  74, 235, 123, 
    155, 153, 186, 136, 189,  21, 
     20, 233,   7, 223, 123, 251, 
    189, 153, 125, 243, 102,  38, 
    217, 153,  31,  14, 196, 168, 
    124, 179,  94, 144, 223, 120, 
    245, 100, 109, 157,   1, 164, 
    240,  25,  93, 236,   9, 117, 
     78,  22, 224, 139,  77,  22, 
     96, 128,  12, 117, 208,   0, 
      2, 116,  71, 144,  30, 234, 
     97, 244, 212, 166, 216, 198, 
    112, 235,  13, 218,  12, 178, 
      3, 249,   9, 223,  25,  69, 
    127,  17, 169,  35,   7, 145, 
     46, 146, 210,  17, 176, 121, 
      7,   6,  25, 132, 242,  73, 
    116, 147, 145,  56, 132, 164, 
     92,   4, 242, 201,  28, 208, 
     79, 230, 207, 160, 122, 169, 
    254, 221,  64, 181, 169,  92, 
     84,  31, 213,  73, 160, 249, 
    254, 109, 180, 215, 171, 109, 
    213, 220, 213,  62,  17, 104, 
     63, 146, 160, 121, 170, 208, 
     99, 185, 143, 106,  93,  87, 
    165, 166,  62,  52, 183,  22, 
    114,  55, 108,  35, 146, 107, 
     79,  80, 154, 242, 153, 114, 
     20, 147,  29, 133,  62, 242, 
    218, 233, 120, 157,  77, 253, 
     50,  76, 161, 255, 223, 161, 
    214, 148,  60, 253, 118, 210, 
    200,  16, 169,  64, 241,  75, 
     72, 128, 229,  38, 241, 105, 
     55, 124,  39, 165,  52, 251, 
     77, 235, 164, 227,  37,  77, 
    104, 250,  99, 234,  48, 169, 
    185, 218, 100, 152, 149, 237, 
     58,  68, 168, 163, 132, 158, 
     64,  61, 145, 208,  83, 168, 
    167,  18, 122,  26, 245, 244, 
    142, 150, 249, 195, 120,  60, 
     45,  69, 251, 140, 193, 165, 
    127, 153, 251,  53, 238,  13, 
    247, 107, 220,  27, 238, 215, 
    184, 103, 140, 177, 100,  60, 
    111,  44,  57,  91,  13,  77, 
    223, 108,   0, 123, 219,  74, 
    179, 143,  45, 240, 184,   0, 
    223,  19, 204, 179,   5, 243, 
    125, 241,  16,  86, 154,  16, 
     46, 111, 104, 230,  75, 224, 
    236, 125,  11, 130, 180, 128, 
     48,  35,  88, 208,  33,  88, 
    216, 181, 211, 110, 104, 239, 
     48, 227, 128,  96, 112,  66, 
    220,  13,  87, 154, 102, 202, 
    227, 150,  21, 112, 110, 167, 
     57, 240, 147,   2, 236,  51, 
      2, 156,  33,  97, 187, 157, 
    220, 241,  13, 207,   9,  77, 
    207, 233, 224,  30, 184, 221, 
    194,  57, 224, 120, 140, 159, 
     18, 204, 206,  11, 230, 156, 
     19,  96, 100,   5, 164, 250, 
      4, 152, 199,   5, 115, 123, 
      4,  88,  32, 180, 131,  70, 
    160, 247, 152, 129, 209, 203, 
      3, 102,  28,  22,  44, 117, 
     76,  48, 179,  95, 104,  62, 
    198, 125, 140, 251,  60,  72, 
    249,  78, 192,  44,  38,  44, 
    215, 240, 184, 107, 122, 182, 
     75, 249,  29,  79, 115,  33, 
    208,  93,  45,  48,  92,  35, 
    200,  26,  91,  13, 252,  54, 
    183, 191, 237,  25, 245, 109, 
    126, 212, 236,  65,  27,  63, 
     34, 212, 157,  68,   1, 143, 
    253, 184, 137,  12,  93,   1, 
     72, 199,   1,   4, 157,  63, 
    120, 230, 196, 154,  72, 247, 
     48, 210, 155, 242, 204, 255, 
     42, 189, 236, 135, 119,   0, 
    234, 245,  11,  89, 121,  71, 
    204,  72,  79, 249, 233,  30, 
     84, 235, 250, 188, 250,  33, 
    187, 206, 207, 203, 248, 144, 
    244, 148, 131,  30, 123, 183, 
     59, 159, 189, 150, 177,  81, 
    233, 103, 145, 244, 126, 233, 
     66,  52,  93,  40,  23,  30, 
    212, 114, 139,  11, 213, 133, 
     59, 185, 218,  98, 181,  88, 
    152,  31, 140, 102, 139, 247, 
    163,  98, 125, 177,  88, 173, 
     20, 202, 209, 173,  82, 125, 
     40,  26, 171, 206,  68,  55, 
    139, 149, 217,  98,  53,  26, 
     57,  59, 146, 175,  71,  99, 
    115,   5,  20, 181, 232, 218, 
    120, 177,  62, 126,  61,  54, 
     87, 235, 165, 133, 234, 252, 
    149, 153,  27, 247, 202, 185, 
     82,  57, 159, 207, 205, 149, 
    107, 101,  28, 165, 141,  53, 
     57,  54,  29, 153,  22, 114, 
    190, 112, 187,   2,  63,   1, 
    145,  50,  63,  81,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     12,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
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
      0,   0,   0,   0
};
