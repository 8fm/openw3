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
//   float4 fsize;                      // Offset:    0 Size:    16
//   float4 offset;                     // Offset:   16 Size:    16
//   float4 srctexscale;                // Offset:   32 Size:    16
//   float4 texscale;                   // Offset:   48 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// sampler_gradtex                   sampler      NA          NA    0        1
// sampler_srctex                    sampler      NA          NA    1        1
// sampler_tex                       sampler      NA          NA    2        1
// gradtex                           texture  float4          2d    0        1
// srctex                            texture  float4          2d    1        1
// tex                               texture  float4          2d    2        1
// Constants                         cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// TEXCOORD                 1   xyzw        1     NONE   float   xyzw
// TEXCOORD                 2   xy          2     NONE   float   xy  
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
// First Precompiled Shader at offset:[434]
// Embedded Data:
//  0x000001b2 - Offset To First Precompiled Shader
//  0x00000050 - Original Shader Descriptor
//  0x000001ad - Original Shader Size
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[4], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v2.xy
dcl_output o0.xyzw
dcl_temps 4
mov r0.xy, l(0,0,0,0)
mov r0.z, -cb0[0].x
loop 
  lt r0.w, cb0[0].x, r0.z
  breakc_nz r0.w
  add r1.x, r0.z, cb0[1].x
  mov r2.xy, r0.xyxx
  mov r2.z, -cb0[0].y
  loop 
    lt r0.w, cb0[0].y, r2.z
    breakc_nz r0.w
    add r1.y, r2.z, cb0[1].y
    mad r1.zw, r1.xxxy, cb0[3].xxxy, v2.xxxy
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.zwzz, t2.xyzw, s2, l(0.000000)
    add r2.y, r0.w, r2.y
    mad r1.yz, -r1.xxyx, cb0[3].xxyx, v2.xxyx
    sample_l_indexable(texture2d)(float,float,float,float) r0.w, r1.yzyy, t2.xyzw, s2, l(0.000000)
    add r2.x, r0.w, r2.x
    add r2.z, r2.z, l(1.000000)
  endloop 
  mov r0.xy, r2.xyxx
  add r0.z, r0.z, l(1.000000)
endloop 
mad r0.xy, -r0.xyxx, cb0[0].wwww, l(1.000000, 1.000000, 0.000000, 0.000000)
add r0.xy, -r0.yxyy, r0.xyxx
mul r0.y, r0.y, cb0[0].z
mad r0.x, -r0.x, cb0[0].z, l(1.000000)
mul r0.x, r0.x, l(0.500000)
max r0.x, r0.x, l(0.000000)
min r1.x, r0.x, l(0.500000)
mov r1.yw, l(0,0,0,0)
sample_indexable(texture2d)(float,float,float,float) r2.xyzw, r1.xyxx, t0.xyzw, s0
mad r0.x, r0.y, l(0.500000), l(0.500000)
max r0.x, r0.x, l(0.500000)
min r1.z, r0.x, l(1.000000)
sample_indexable(texture2d)(float,float,float,float) r0.xyzw, r1.zwzz, t0.xyzw, s0
mul r1.xy, v2.xyxx, cb0[2].xyxx
sample_l_indexable(texture2d)(float,float,float,float) r1.xyzw, r1.xyxx, t1.xyzw, s1, l(0.000000)
mov_sat r3.x, r2.w
mov_sat r3.y, r0.w
mov r2.w, l(1.000000)
mov r0.w, l(1.000000)
mul r0.xyzw, r3.yyyy, r0.xyzw
mad r0.xyzw, r2.xyzw, r3.xxxx, r0.xyzw
add r2.x, -r3.x, l(1.000000)
add r2.x, -r3.y, r2.x
mad r0.xyzw, r1.xyzw, r2.xxxx, r0.xyzw
mul r0.xyzw, r1.wwww, r0.xyzw
mov r1.xyz, v1.xyzx
mov r1.w, l(1.000000)
mul r0.xyzw, r0.xyzw, r1.xyzw
mul r0.xyzw, r0.xyzw, v1.wwww
mad o0.xyzw, v0.xyzw, r0.wwww, r0.xyzw
ret 
Opaque Custom Data - XBOX Precompiled Shader
// Offset:[434], bundle is:[246] DWORDS.
// Disassembled Precompiled Shader Object:
// Shader Type               : (PS)
// Shader Hw Stage           : (PS)
// HLSL Source Filename      : f:\balazs.torok.stream2\dev\external\gfx4\Src\Render\D3D1x\Shaders\FSGrad2InnerBevel.fl11.hlsl
// HLSL Entrypoint Name      : main

//
// Shader Data:
// ----------------- PS Data ------------------------
; Input Semantic Mappings
;   [0] generic, usageIdx  0, channelMask 15, param0, paramSlot 0, DefaultVal={0,0,0,0};   [1] generic, usageIdx  1, channelMask 15, param1, paramSlot 1, DefaultVal={0,0,0,0};   [2] generic, usageIdx  2, channelMask  3, param2, paramSlot 2, DefaultVal={0,0,0,0}

codeLenInByte        = 568;Bytes

; launchModeFlags    = 0x0000000E
srdTableSupport      = TRUE
immediateSupportExt  = TRUE
srdTableSupportExt   = TRUE

userElementCount     = 1;
;  userElements[0]      = PTR_EXTENDED_USER_DATA, ReferencedExtUserElements=1, s[0:1]
extUserElementCount  = 7;
;  extUserElements 1[0] = IMM_RESOURCE, 0, offset 0:7 dwords
;  extUserElements 1[1] = IMM_RESOURCE, 1, offset 8:15 dwords
;  extUserElements 1[2] = IMM_RESOURCE, 2, offset 16:23 dwords
;  extUserElements 1[3] = IMM_SAMPLER, 0, offset 24:27 dwords
;  extUserElements 1[4] = IMM_SAMPLER, 1, offset 28:31 dwords
;  extUserElements 1[5] = IMM_SAMPLER, 2, offset 32:35 dwords
;  extUserElements 1[6] = IMM_CONST_BUFFER, 0, offset 36:39 dwords
NumVgprs             = 15;
NumSgprs             = 34;
FloatMode            = 192;
IeeeMode             = 0;
bFlatPtr32           = 0;
ScratchSize          = 0 dwords/thread;
LDSByteSize          = 0 bytes/workgroup (compile time only);
ScratchWaveOffsetReg = s65535;
; texSamplerUsage         = 0x00000007
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
; SPI_PS_IN_CONTROL       = 0x00000003
SPIC:NUM_INTERP             = 3
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
  s_mov_b64     s[32:33], exec                          // 000000000000: BEA0047E
  s_wqm_b64     exec, exec                              // 000000000004: BEFE0A7E
  s_mov_b32     m0, s2                                  // 000000000008: BEFC0302
  s_load_dwordx8  s[4:11], s[0:1], 0x20                 // 00000000000C: C0C20120
  s_waitcnt     lgkmcnt(0)                              // 000000000010: BF8C007F
  s_buffer_load_dwordx4  s[12:15], s[8:11], 0x00        // 000000000014: C2860900
  v_mov_b32     v2, 0                                   // 000000000018: 7E040280
  v_mov_b32     v3, 0                                   // 00000000001C: 7E060280
  s_load_dwordx8  s[16:23], s[0:1], 0x10                // 000000000020: C0C80110
  s_waitcnt     lgkmcnt(0)                              // 000000000024: BF8C007F
  v_max_f32     v4, -s12, -s12                          // 000000000028: D2200004 6000180C
label_000C:
  v_mov_b32     v5, s12                                 // 000000000030: 7E0A020C
  v_cmp_gt_f32  vcc, v4, v5                             // 000000000034: 7C080B04
  s_cbranch_vccnz  label_0032                           // 000000000038: BF870023
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x04          // 00000000003C: C2410904
  s_waitcnt     lgkmcnt(0)                              // 000000000040: BF8C007F
  v_add_f32     v5, s2, v4                              // 000000000044: 060A0802
  v_max_f32     v6, -s13, -s13                          // 000000000048: D2200006 60001A0D
label_0014:
  v_mov_b32     v7, s13                                 // 000000000050: 7E0E020D
  v_cmp_gt_f32  vcc, v6, v7                             // 000000000054: 7C080F06
  s_cbranch_vccnz  label_0030                           // 000000000058: BF870019
  s_buffer_load_dwordx2  s[24:25], s[8:11], 0x0c        // 00000000005C: C24C090C
  v_interp_p1_f32  v7, v0, attr2.x                      // 000000000060: C81C0800
  v_interp_p1_f32  v8, v0, attr2.y                      // 000000000064: C8200900
  v_interp_p2_f32  v7, v1, attr2.x                      // 000000000068: C81D0801
  v_interp_p2_f32  v8, v1, attr2.y                      // 00000000006C: C8210901
  v_add_f32     v9, s3, v6                              // 000000000070: 06120C03
  s_waitcnt     lgkmcnt(0)                              // 000000000074: BF8C007F
  v_mad_legacy_f32  v10, v5, s24, v7                    // 000000000078: D280000A 041C3105
  v_mad_legacy_f32  v11, v9, s25, v8                    // 000000000080: D280000B 04203309
  v_mad_legacy_f32  v7, -v5, s24, v7                    // 000000000088: D2800007 241C3105
  v_mad_legacy_f32  v8, -v9, s25, v8                    // 000000000090: D2800008 24203309
  image_sample_lz  v[9:12], v[10:11], s[16:23], s[4:7] dmask:0x8 // 000000000098: F09C0800 0024090A
  image_sample_lz  v[7:10], v[7:8], s[16:23], s[4:7] dmask:0x8 // 0000000000A0: F09C0800 00240707
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 0000000000A8: BF8C0F71
  v_add_f32     v3, v3, v9                              // 0000000000AC: 06061303
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 0000000000B0: BF8C0F70
  v_add_f32     v2, v2, v7                              // 0000000000B4: 06040F02
  v_add_f32     v6, 1.0, v6                             // 0000000000B8: 060C0CF2
  s_branch      label_0014                              // 0000000000BC: BF82FFE4
label_0030:
  v_add_f32     v4, 1.0, v4                             // 0000000000C0: 060808F2
  s_branch      label_000C                              // 0000000000C4: BF82FFDA
label_0032:
  v_mad_legacy_f32  v2, -v2, s15, 1.0                   // 0000000000C8: D2800002 23C81F02
  v_mad_legacy_f32  v3, -v3, s15, 1.0                   // 0000000000D0: D2800003 23C81F03
  v_sub_f32     v4, v3, v2                              // 0000000000D8: 08080503
  v_sub_f32     v2, v2, v3                              // 0000000000DC: 08040702
  s_load_dwordx8  s[16:23], s[0:1], 0x00                // 0000000000E0: C0C80100
  s_load_dwordx8  s[24:31], s[0:1], 0x18                // 0000000000E4: C0CC0118
  v_mul_legacy_f32  v3, s14, v4                         // 0000000000E8: 0E06080E
  v_mad_legacy_f32  v2, -v2, s14, 1.0 div:2             // 0000000000EC: D2800002 3BC81D02
  v_mad_legacy_f32  v3, v3, 0.5, 0.5                    // 0000000000F4: D2800003 03C1E103
  v_med3_f32    v6, v2, 0, 0.5                          // 0000000000FC: D2AE0006 03C10102
  v_med3_f32    v2, v3, 0.5, 1.0                        // 000000000104: D2AE0002 03C9E103
  v_mov_b32     v3, 0                                   // 00000000010C: 7E060280
  s_waitcnt     lgkmcnt(0)                              // 000000000110: BF8C007F
  image_sample  v[10:13], v[2:3], s[16:23], s[24:27] dmask:0xf // 000000000114: F0800F00 00C40A02
  s_buffer_load_dwordx2  s[2:3], s[8:11], 0x08          // 00000000011C: C2410908
  v_interp_p1_f32  v4, v0, attr2.x                      // 000000000120: C8100800
  v_interp_p1_f32  v5, v0, attr2.y                      // 000000000124: C8140900
  s_load_dwordx8  s[4:11], s[0:1], 0x08                 // 000000000128: C0C20108
  v_mov_b32     v7, 0                                   // 00000000012C: 7E0E0280
  v_interp_p2_f32  v4, v1, attr2.x                      // 000000000130: C8110801
  v_interp_p2_f32  v5, v1, attr2.y                      // 000000000134: C8150901
  image_sample  v[6:9], v[6:7], s[16:23], s[24:27] dmask:0xf // 000000000138: F0800F00 00C40606
  s_waitcnt     lgkmcnt(0)                              // 000000000140: BF8C007F
  v_mul_legacy_f32  v2, s2, v4                          // 000000000144: 0E040802
  v_mul_legacy_f32  v3, s3, v5                          // 000000000148: 0E060A03
  image_sample_lz  v[2:5], v[2:3], s[4:11], s[28:31] dmask:0xf // 00000000014C: F09C0F00 00E10202
  s_waitcnt     vmcnt(2) & lgkmcnt(15)                  // 000000000154: BF8C0F72
  v_max_f32     v13, v13, v13 clamp                     // 000000000158: D220080D 00021B0D
  s_waitcnt     vmcnt(1) & lgkmcnt(15)                  // 000000000160: BF8C0F71
  v_max_f32     v9, v9, v9 clamp                        // 000000000164: D2200809 00021309
  v_mul_legacy_f32  v10, v10, v13                       // 00000000016C: 0E141B0A
  v_mul_legacy_f32  v11, v11, v13                       // 000000000170: 0E161B0B
  v_sub_f32     v14, 1.0, v9                            // 000000000174: 081C12F2
  v_mac_legacy_f32  v10, v6, v9                         // 000000000178: 0C141306
  v_mac_legacy_f32  v11, v7, v9                         // 00000000017C: 0C161307
  v_subrev_f32  v6, v13, v14                            // 000000000180: 0A0C1D0D
  s_waitcnt     vmcnt(0) & lgkmcnt(15)                  // 000000000184: BF8C0F70
  v_mac_legacy_f32  v10, v2, v6                         // 000000000188: 0C140D02
  v_mac_legacy_f32  v11, v3, v6                         // 00000000018C: 0C160D03
  v_mul_legacy_f32  v12, v12, v13                       // 000000000190: 0E181B0C
  v_interp_p1_f32  v2, v0, attr1.x                      // 000000000194: C8080400
  v_interp_p1_f32  v3, v0, attr1.y                      // 000000000198: C80C0500
  v_mac_legacy_f32  v12, v8, v9                         // 00000000019C: 0C181308
  v_add_f32     v7, v9, v13                             // 0000000001A0: 060E1B09
  v_mac_legacy_f32  v12, v4, v6                         // 0000000001A4: 0C180D04
  v_mac_legacy_f32  v7, v5, v6                          // 0000000001A8: 0C0E0D05
  v_interp_p2_f32  v2, v1, attr1.x                      // 0000000001AC: C8090401
  v_interp_p2_f32  v3, v1, attr1.y                      // 0000000001B0: C80D0501
  v_interp_p1_f32  v4, v0, attr1.z                      // 0000000001B4: C8100600
  v_mul_legacy_f32  v6, v5, v10                         // 0000000001B8: 0E0C1505
  v_mul_legacy_f32  v8, v5, v11                         // 0000000001BC: 0E101705
  v_mul_legacy_f32  v9, v5, v12                         // 0000000001C0: 0E121905
  v_interp_p1_f32  v10, v0, attr1.w                     // 0000000001C4: C8280700
  v_mul_legacy_f32  v5, v5, v7                          // 0000000001C8: 0E0A0F05
  v_mul_legacy_f32  v2, v6, v2                          // 0000000001CC: 0E040506
  v_mul_legacy_f32  v3, v8, v3                          // 0000000001D0: 0E060708
  v_interp_p2_f32  v4, v1, attr1.z                      // 0000000001D4: C8110601
  v_interp_p1_f32  v6, v0, attr0.x                      // 0000000001D8: C8180000
  v_interp_p1_f32  v7, v0, attr0.y                      // 0000000001DC: C81C0100
  v_interp_p1_f32  v8, v0, attr0.z                      // 0000000001E0: C8200200
  v_interp_p2_f32  v10, v1, attr1.w                     // 0000000001E4: C8290701
  v_interp_p1_f32  v0, v0, attr0.w                      // 0000000001E8: C8000300
  v_mul_legacy_f32  v4, v9, v4                          // 0000000001EC: 0E080909
  v_interp_p2_f32  v6, v1, attr0.x                      // 0000000001F0: C8190001
  v_interp_p2_f32  v7, v1, attr0.y                      // 0000000001F4: C81D0101
  v_interp_p2_f32  v8, v1, attr0.z                      // 0000000001F8: C8210201
  v_mul_legacy_f32  v2, v2, v10                         // 0000000001FC: 0E041502
  v_mul_legacy_f32  v3, v3, v10                         // 000000000200: 0E061503
  v_mul_legacy_f32  v4, v4, v10                         // 000000000204: 0E081504
  v_mul_legacy_f32  v5, v5, v10                         // 000000000208: 0E0A1505
  v_interp_p2_f32  v0, v1, attr0.w                      // 00000000020C: C8010301
  v_mac_legacy_f32  v2, v6, v5                          // 000000000210: 0C040B06
  v_mac_legacy_f32  v3, v7, v5                          // 000000000214: 0C060B07
  v_mac_legacy_f32  v4, v8, v5                          // 000000000218: 0C080B08
  v_mac_legacy_f32  v5, v0, v5                          // 00000000021C: 0C0A0B00
  s_mov_b64     exec, s[32:33]                          // 000000000220: BEFE0420
  v_cvt_pkrtz_f16_f32  v0, v2, v3                       // 000000000224: 5E000702
  v_cvt_pkrtz_f16_f32  v1, v4, v5                       // 000000000228: 5E020B04
  exp           mrt0, v0, v0, v1, v1 compr vm           // 00000000022C: F800140F 00000100
  s_endpgm                                              // 000000000234: BF810000
end


// Approximately 54 instruction slots used
#endif

const BYTE pBinary_D3D1xFL11X_FSGrad2InnerBevel[] =
{
     68,  88,  66,  67,  96, 166, 
     21, 191,  57, 228,  97, 216, 
    187, 111, 136, 181, 171, 216, 
      1, 109,   1,   0,   0,   0, 
    200,  14,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    228,   2,   0,   0,  72,   3, 
      0,   0, 124,   3,   0,   0, 
     44,  14,   0,   0,  82,  68, 
     69,  70, 168,   2,   0,   0, 
      1,   0,   0,   0, 100,   1, 
      0,   0,   7,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0, 129,   0,   0, 
    108,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
     28,   1,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  44,   1,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,  59,   1, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     71,   1,   0,   0,   2,   0, 
      0,   0,   5,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0,  79,   1,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   1,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0,  86,   1, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      2,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
     90,   1,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114,  95, 103, 114, 
     97, 100, 116, 101, 120,   0, 
    115,  97, 109, 112, 108, 101, 
    114,  95, 115, 114,  99, 116, 
    101, 120,   0, 115,  97, 109, 
    112, 108, 101, 114,  95, 116, 
    101, 120,   0, 103, 114,  97, 
    100, 116, 101, 120,   0, 115, 
    114,  99, 116, 101, 120,   0, 
    116, 101, 120,   0,  67, 111, 
    110, 115, 116,  97, 110, 116, 
    115,   0,  90,   1,   0,   0, 
      4,   0,   0,   0, 124,   1, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  28,   2,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     80,   2,   0,   0,  16,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  44,   2, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  87,   2, 
      0,   0,  32,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0,  44,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  99,   2,   0,   0, 
     48,   0,   0,   0,  16,   0, 
      0,   0,   2,   0,   0,   0, 
     44,   2,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    102, 115, 105, 122, 101,   0, 
    102, 108, 111,  97, 116,  52, 
      0, 171, 171, 171,   1,   0, 
      3,   0,   1,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     34,   2,   0,   0, 111, 102, 
    102, 115, 101, 116,   0, 115, 
    114,  99, 116, 101, 120, 115, 
     99,  97, 108, 101,   0, 116, 
    101, 120, 115,  99,  97, 108, 
    101,   0,  77, 105,  99, 114, 
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
     92,   0,   0,   0,   3,   0, 
      0,   0,   8,   0,   0,   0, 
     80,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     80,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
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
     69,  88, 168,  10,   0,   0, 
     80,   0,   0,   0, 170,   2, 
      0,   0,  53,  16,   0,   0, 
      5,   0,   0,   0, 178,   1, 
      0,   0,  80,   0,   0,   0, 
    173,   1,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   0,   0,   0,   0, 
     98,  16,   0,   3, 242,  16, 
     16,   0,   1,   0,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   4,   0, 
      0,   0,  54,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     49,   0,   0,   8, 130,   0, 
     16,   0,   0,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,   3,   0,   4,   3, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  10, 128, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  54,   0, 
      0,   5,  50,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  66,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  48,   0, 
      0,   1,  49,   0,   0,   8, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  26, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0,   3,   0, 
      4,   3,  58,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   8,  34,   0,  16,   0, 
      1,   0,   0,   0,  42,   0, 
     16,   0,   2,   0,   0,   0, 
     26, 128,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     50,   0,   0,  10, 194,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   4,  16,   0,   1,   0, 
      0,   0,   6, 132,  32,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 130,   0, 
     16,   0,   0,   0,   0,   0, 
    230,  10,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      2,   0,   0,   0,   0,  96, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   2,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,  11,  98,   0, 
     16,   0,   1,   0,   0,   0, 
      6,   1,  16, 128,  65,   0, 
      0,   0,   1,   0,   0,   0, 
      6, 129,  32,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      6,  17,  16,   0,   2,   0, 
      0,   0,  72,   0,   0, 141, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 130,   0,  16,   0, 
      0,   0,   0,   0, 150,   5, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  18,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   7,  66,   0,  16,   0, 
      2,   0,   0,   0,  42,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  22,   0,   0,   1, 
     54,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
     66,   0,  16,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     22,   0,   0,   1,  50,   0, 
      0,  14,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0, 246, 143, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   8,  50,   0, 
     16,   0,   0,   0,   0,   0, 
     22,   5,  16, 128,  65,   0, 
      0,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  42, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  50,   0, 
      0,  11,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,  42, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     56,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  51,   0,   0,   7, 
     18,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     54,   0,   0,   8, 162,   0, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  69,   0,   0, 139, 
    194,   0,   0, 128,  67,  85, 
     21,   0, 242,   0,  16,   0, 
      2,   0,   0,   0,  70,   0, 
     16,   0,   1,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0,   0,  96,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  18,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,   1,  64,   0,   0, 
      0,   0,   0,  63,  52,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,  51,   0,   0,   7, 
     66,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     69,   0,   0, 139, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0, 230,  10,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   8, 
     50,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      2,   0,   0,   0,  70, 128, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  72,   0, 
      0, 141, 194,   0,   0, 128, 
     67,  85,  21,   0, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   0,  16,   0,   1,   0, 
      0,   0,  70, 126,  16,   0, 
      1,   0,   0,   0,   0,  96, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  54,  32,   0,   5, 
     18,   0,  16,   0,   3,   0, 
      0,   0,  58,   0,  16,   0, 
      2,   0,   0,   0,  54,  32, 
      0,   5,  34,   0,  16,   0, 
      3,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  54,   0,   0,   5, 
    130,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  56,   0, 
      0,   7, 242,   0,  16,   0, 
      0,   0,   0,   0,  86,   5, 
     16,   0,   3,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,   9, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      2,   0,   0,   0,   6,   0, 
     16,   0,   3,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   8, 
     18,   0,  16,   0,   2,   0, 
      0,   0,  10,   0,  16, 128, 
     65,   0,   0,   0,   3,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,   8,  18,   0,  16,   0, 
      2,   0,   0,   0,  26,   0, 
     16, 128,  65,   0,   0,   0, 
      3,   0,   0,   0,  10,   0, 
     16,   0,   2,   0,   0,   0, 
     50,   0,   0,   9, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,   6,   0,  16,   0, 
      2,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 114,   0,  16,   0, 
      1,   0,   0,   0,  70,  18, 
     16,   0,   1,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  56,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     56,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0, 246,  31,  16,   0, 
      1,   0,   0,   0,  50,   0, 
      0,   9, 242,  32,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   0,   0,   0,   0, 
    246,  15,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  53,  16,   0,   0, 
    248,   0,   0,   0,  21,   0, 
      1,   0,  41,  80,  12,   0, 
    120,   1, 237,  86, 207, 107, 
     92,  69,  28, 255, 206, 188, 
    121, 243, 230, 189,  55,  59, 
    111, 155, 221, 141,  27, 119, 
    151, 110, 155, 168,  13,  72, 
     48, 109,  15, 162,  23, 149, 
    224,  15, 240,  32,   6, 177, 
    148,  71, 155, 141, 217, 216, 
    226, 118, 219, 110,  66,  88, 
     66,  89,  87,  15, 138, 208, 
     67,   5, 241, 212, 131,  87, 
    161, 136,   7, 241, 152,  77, 
     66,  14,  35, 120, 202,  77, 
    111,   1, 255, 128, 144, 147, 
      7, 177, 126, 191, 239,   7, 
     89, 193, 222,   2,  10, 230, 
      3, 159, 249, 206, 103, 230, 
     59,  51, 223, 249,   1, 243, 
    125,  71,  67, 130, 234, 179, 
    209,   4, 217, 231, 239,  54, 
    110, 254,  32,   1,  92, 172, 
     51,  36,  89,  66,  95,  81, 
      9, 240,  66,  72,  37, 192, 
      5,  42, 200, 129,   3,  68, 
    104, 206,  35,  95, 196, 250, 
     43, 104, 169,  47, 103,  10, 
    187, 179, 128, 190, 151, 177, 
     70, 126, 155,  56, 233,   6, 
    218,  85, 100,  25, 249,  62, 
    178, 129, 164, 233,   8, 249, 
    154, 132,  34,  21,   8,  65, 
    197,  24,  48,  68, 152,  69, 
     26,  36,  46,  11,  85, 164, 
    143,  36,  80, 223, 223, 252, 
    159, 161, 226, 159, 177, 137, 
    164, 248, 159,   4, 138,  45, 
    143, 135, 226, 163,  56,   9, 
    180, 143, 127,  27, 233, 121, 
     57,  84, 128, 135,  36, 157, 
    223,  19, 129, 238,  99,  28, 
    116,  86, 233,  24, 128, 239, 
      3,  42, 211, 113, 132, 237, 
     76, 231, 119, 185, 141, 124, 
     18,  30,  35, 142, 239,  54, 
     69, 174,  41,  26, 153, 157, 
    168, 100, 103, 161,  73, 150, 
     79,  39, 239, 226, 255, 130, 
    252, 237, 209,  89, 208,  21, 
    208,  27,  78,  79,  36,   5, 
    245, 127, 129,   4,  24, 142, 
    136,  15, 170, 240,  59, 169, 
     92,  51, 212, 233, 213, 165, 
    218,  33,  77, 147, 141, 107, 
    124, 148, 220, 131, 107,  34, 
    228, 215, 162,  58, 142, 199, 
    139,  69,  29, 163, 142, 199, 
    244,  85, 212,  87, 199, 244, 
     18, 234, 165,  49, 189, 140, 
    122,  57, 215, 249, 252,  81, 
    178, 158, 112, 233, 158, 177, 
    113, 240,  95, 230, 105, 140, 
     39, 195, 211,  24,  79, 134, 
    167,  49, 158,  24,  19,  12, 
    196,  55,  91, 131, 224, 207, 
     45, 238, 252, 177, 213, 100, 
    187, 219,  31, 195, 253,  17, 
    248, 159, 237,  14, 185,  24, 
     12, 185,  28,  20, 153,  77, 
    218,   4,  52, 247, 117,  21, 
    150,  52,  15,   6,  34,  84, 
    247, 166, 225, 243, 145, 240, 
     95, 221, 165,  62, 174,   2, 
     41, 177, 191, 240,  52,  44, 
     21, 184,  25, 200,  72, 221, 
    155, 194, 126, 237, 191, 189, 
     11, 170, 110, 193, 111,  90, 
    166,  26, 150, 249, 231, 172, 
    163,  39,  36, 141,   9,  96, 
    184, 239, 206, 215,  69, 136, 
    214, 191, 212,  20,  94, 170, 
    103,  84, 170, 103,  64,  61, 
     60,  12, 252,  25,  32, 235, 
    121,  51, 112,  55, 186,  63, 
    114,  74,  82, 222,  65, 203, 
     35,  33, 143, 180, 150, 191, 
     61, 254, 116, 116, 164, 148, 
    252,  21,  45, 199, 113, 252, 
    172, 157, 118, 208,  58, 100, 
     93, 165, 184,  39,  20,  96, 
    252,  85, 246, 243, 182,  81, 
    210,  36,  62,  13, 251, 114, 
    226, 115, 176, 227,  72, 248, 
    110, 159, 179,  29, 135, 163, 
    117,  14, 126, 114, 104, 191, 
     20,  27,  68, 195,  67,  30, 
    236, 129, 194, 253, 129,  42, 
     98, 252, 101, 171, 240, 108, 
    134, 184,  55, 166, 206, 224, 
     62,  42, 150, 124, 164, 220, 
      3, 242, 231,  74,  24,  39, 
    144,   6, 162, 135, 135, 156, 
     31,  64,  15,  99,  44,  40, 
     60, 143,  26,  79, 226, 246, 
    177, 238, 151,  56,   4, 181, 
    178,   9, 107, 147, 230, 104, 
    162, 174, 100, 169, 172, 189, 
    210, 164,  46,  52, 116, 144, 
    236, 169,  80, 214,  78,  97, 
     82, 235,  90, 213, 128,  80, 
     22,  92, 109,  85, 169, 170, 
    253, 154, 145, 162,  80, 213, 
    110, 193, 104,  38, 124, 203, 
    220, 130,   5,  89, 180, 110, 
     69,  27, 247, 169, 162, 113, 
    167,  38,  12, 120,  23, 172, 
     27,   5,  70, 186, 194,  40, 
     79,  26,  38, 207,  88, 204, 
    216,  44,  48,  60, 123, 142, 
    103, 239, 205,  90, 252,  44, 
    173, 239,  43, 195,  96, 202, 
     50, 134, 119, 193, 207,  89, 
     94, 193, 184,  43, 210, 136, 
    138,  50, 110,  37,  48, 204, 
     97,  86, 134,  66, 123, 161, 
    212,  42,  84,  26, 194,  64, 
     55,   5, 190, 141, 252,  31, 
     44, 231, 255, 216,  39, 163, 
     41,  44, 147,  42,  34, 253, 
     75, 143, 129,  41,  82, 210, 
     69,   5, 165,  75, 185,  38, 
     63, 250, 171,  73,  39,  13, 
      8, 202,  43, 241, 191,  78, 
     52, 177, 142, 204,  53, 249, 
     83, 126,  67, 154, 234,   4, 
    124,  17, 137, 190, 146, 229, 
    201, 111, 102, 150, 230, 197, 
    118, 204, 193, 104, 228,  49, 
    250,  89, 255, 157, 204, 226, 
    218,  64, 213,  47,  31, 125, 
    187, 215, 252, 101, 246, 235, 
    175, 178, 246,   7, 153, 165, 
     57,  40,  85, 123, 238, 202, 
    123,  63, 134,  89, 238, 254, 
     40, 235, 187, 142, 164, 241, 
    171,  47, 197, 203, 173,  78, 
    107, 115, 109, 110, 253, 118, 
    239, 246,  71, 115, 107, 235, 
    189, 118, 235, 214, 197, 120, 
    165, 189,  17, 183, 251, 235, 
    237,  94, 183, 213, 137,  63, 
     92, 237,  95, 142,  23, 123, 
     31, 196, 239, 182, 187,  43, 
    237,  94, 188, 112, 105,  97, 
    190,  31,  47, 222, 104, 161, 
     88, 139,  95,  95, 124, 163, 
    215,  90, 185, 248,  86, 183, 
    219, 238, 189, 214, 222, 104, 
    119, 230,  86,  59, 243, 243, 
    115,  55,  58, 107,  29,  92, 
     32, 197, 116, 182,  60, 165, 
    199, 148,  70, 222, 106, 221, 
    236, 194,  95,  65, 247, 134, 
     60,   0,   0,   0,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     54,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,  30,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  12,   0, 
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
