/**************************************************************************
 *
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef CAIRO_DRM_INTEL_BRW_STRUCTS_H
#define CAIRO_DRM_INTEL_BRW_STRUCTS_H
// 
// Command packets:
// 
struct header {
    uint length:16;
    uint opcode:16;
};

union header_union {
    struct header bits;
    uint dword;
};

struct brw_3d_control {
    struct {
	uint length:8;
	uint notify_enable:1;
	uint pad:3;
	uint wc_flush_enable:1;
	uint depth_stall_enable:1;
	uint operation:2;
	uint opcode:16;
    } header;

    struct {
	uint pad:2;
	uint dest_addr_type:1;
	uint dest_addr:29;
    } dest;

    uint dword2;
    uint dword3;
};


struct brw_3d_primitive {
    struct {
	uint length:8;
	uint pad:2;
	uint topology:5;
	uint indexed:1;
	uint opcode:16;
    } header;

    uint verts_per_instance;
    uint start_vert_location;
    uint instance_count;
    uint start_instance_location;
    uint base_vert_location;
};

/* These seem to be passed around as function args, so it works out
 * better to keep them as #defines:
 */
#define BRW_FLUSH_READ_CACHE           0x1
#define BRW_FLUSH_STATE_CACHE          0x2
#define BRW_INHIBIT_FLUSH_RENDER_CACHE 0x4
#define BRW_FLUSH_SNAPSHOT_COUNTERS    0x8

struct brw_mi_flush {
    uint flags:4;
    uint pad:12;
    uint opcode:16;
};

struct brw_vf_statistics {
    uint statistics_enable:1;
    uint pad:15;
    uint opcode:16;
};


struct brw_binding_table_pointers {
    struct header header;
    uint vs;
    uint gs;
    uint clp;
    uint sf;
    uint wm;
};

struct brw_blend_constant_color {
    struct header header;
    float blend_constant_color[4];
};

struct brw_depthbuffer {
    union header_union header;

    union {
	struct {
	    uint pitch:18;
	    uint format:3;
	    uint pad:4;
	    uint depth_offset_disable:1;
	    uint tile_walk:1;
	    uint tiled_surface:1;
	    uint pad2:1;
	    uint surface_type:3;
	} bits;
	uint dword;
    } dword1;

    uint dword2_base_addr;

    union {
	struct {
	    uint pad:1;
	    uint mipmap_layout:1;
	    uint lod:4;
	    uint width:13;
	    uint height:13;
	} bits;
	uint dword;
    } dword3;

    union {
	struct {
	    uint pad:12;
	    uint min_array_element:9;
	    uint depth:11;
	} bits;
	uint dword;
    } dword4;
};

struct brw_drawrect {
    struct header header;
    uint xmin:16;
    uint ymin:16;
    uint xmax:16;
    uint ymax:16;
    uint xorg:16;
    uint yorg:16;
};

struct brw_global_depth_offset_clamp {
    struct header header;
    float depth_offset_clamp;
};

struct brw_indexbuffer {
    union {
	struct {
	    uint length:8;
	    uint index_format:2;
	    uint cut_index_enable:1;
	    uint pad:5;
	    uint opcode:16;
	} bits;
	uint dword;
    } header;
    uint buffer_start;
    uint buffer_end;
};


struct brw_line_stipple {
    struct header header;

    struct {
	uint pattern:16;
	uint pad:16;
    } bits0;

    struct {
	uint repeat_count:9;
	uint pad:7;
	uint inverse_repeat_count:16;
    } bits1;
};

struct brw_pipelined_state_pointers {
    struct header header;

    struct {
	uint pad:5;
	uint offset:27;
    } vs;

    struct {
	uint enable:1;
	uint pad:4;
	uint offset:27;
    } gs;

    struct {
	uint enable:1;
	uint pad:4;
	uint offset:27;
    } clp;

    struct {
	uint pad:5;
	uint offset:27;
    } sf;

    struct {
	uint pad:5;
	uint offset:27;
    } wm;

    struct {
	uint pad:6;
	uint offset:26;
    } cc;
};

struct brw_polygon_stipple_offset {
    struct header header;

    struct {
	uint y_offset:5;
	uint pad:3;
	uint x_offset:5;
	uint pad0:19;
    } bits0;
};

struct brw_polygon_stipple {
    struct header header;
    uint stipple[32];
};

struct brw_pipeline_select {
    struct {
	uint pipeline_select:1;
	uint pad:15;
	uint opcode:16;
    } header;
};

struct brw_pipe_control {
    struct {
	uint length:8;
	uint notify_enable:1;
	uint pad:2;
	uint instruction_state_cache_flush_enable:1;
	uint write_cache_flush_enable:1;
	uint depth_stall_enable:1;
	uint post_sync_operation:2;

	uint opcode:16;
    } header;

    struct {
	uint pad:2;
	uint dest_addr_type:1;
	uint dest_addr:29;
    } bits1;

    uint data0;
    uint data1;
};


struct brw_urb_fence {
    struct {
	uint length:8;
	uint vs_realloc:1;
	uint gs_realloc:1;
	uint clp_realloc:1;
	uint sf_realloc:1;
	uint vfe_realloc:1;
	uint cs_realloc:1;
	uint pad:2;
	uint opcode:16;
    } header;

    struct {
	uint vs_fence:10;
	uint gs_fence:10;
	uint clp_fence:10;
	uint pad:2;
    } bits0;

    struct {
	uint sf_fence:10;
	uint vf_fence:10;
	uint cs_fence:10;
	uint pad:2;
    } bits1;
};

struct brw_constant_buffer_state {
    struct header header;

    struct {
	uint nr_urb_entries:3;
	uint pad:1;
	uint urb_entry_size:5;
	uint pad0:23;
    } bits0;
};

struct brw_constant_buffer {
    struct {
	uint length:8;
	uint valid:1;
	uint pad:7;
	uint opcode:16;
    } header;

    struct {
	uint buffer_length:6;
	uint buffer_address:26;
    } bits0;
};

struct brw_state_base_address {
    struct header header;

    struct {
	uint modify_enable:1;
	uint pad:4;
	uint general_state_address:27;
    } bits0;

    struct {
	uint modify_enable:1;
	uint pad:4;
	uint surface_state_address:27;
    } bits1;

    struct {
	uint modify_enable:1;
	uint pad:4;
	uint indirect_object_state_address:27;
    } bits2;

    struct {
	uint modify_enable:1;
	uint pad:11;
	uint general_state_upper_bound:20;
    } bits3;

    struct {
	uint modify_enable:1;
	uint pad:11;
	uint indirect_object_state_upper_bound:20;
    } bits4;
};

struct brw_state_prefetch {
    struct header header;

    struct {
	uint prefetch_count:3;
	uint pad:3;
	uint prefetch_pointer:26;
    } bits0;
};

struct brw_system_instruction_pointer {
    struct header header;

    struct {
	uint pad:4;
	uint system_instruction_pointer:28;
    } bits0;
};


/* State structs for the various fixed function units:
*/

struct thread0 {
    uint pad0:1;
    uint grf_reg_count:3;
    uint pad1:2;
    uint kernel_start_pointer:26;
};

struct thread1 {
    uint ext_halt_exception_enable:1;
    uint sw_exception_enable:1;
    uint mask_stack_exception_enable:1;
    uint timeout_exception_enable:1;
    uint illegal_op_exception_enable:1;
    uint pad0:3;
    uint depth_coef_urb_read_offset:6;	/* WM only */
    uint pad1:2;
    uint floating_point_mode:1;
    uint thread_priority:1;
    uint binding_table_entry_count:8;
    uint pad3:5;
    uint single_program_flow:1;
};

struct thread2 {
    uint per_thread_scratch_space:4;
    uint pad0:6;
    uint scratch_space_base_pointer:22;
};

struct thread3 {
    uint dispatch_grf_start_reg:4;
    uint urb_entry_read_offset:6;
    uint pad0:1;
    uint urb_entry_read_length:6;
    uint pad1:1;
    uint const_urb_entry_read_offset:6;
    uint pad2:1;
    uint const_urb_entry_read_length:6;
    uint pad3:1;
};

struct brw_clip_unit_state {
    struct thread0 thread0;
    struct thread1 thread1;
    struct thread2 thread2;
    struct thread3 thread3;

    struct {
	uint pad0:9;
	uint gs_output_stats:1; /* not always */
	uint stats_enable:1;
	uint nr_urb_entries:7;
	uint pad1:1;
	uint urb_entry_allocation_size:5;
	uint pad2:1;
	uint max_threads:6;	/* may be less */
	uint pad3:1;
    } thread4;

    struct {
	uint pad0:13;
	uint clip_mode:3;
	uint userclip_enable_flags:8;
	uint userclip_must_clip:1;
	uint pad1:1;
	uint guard_band_enable:1;
	uint viewport_z_clip_enable:1;
	uint viewport_xy_clip_enable:1;
	uint vertex_position_space:1;
	uint api_mode:1;
	uint pad2:1;
    } clip5;

    struct {
	uint pad0:5;
	uint clipper_viewport_state_ptr:27;
    } clip6;

    float viewport_xmin;
    float viewport_xmax;
    float viewport_ymin;
    float viewport_ymax;
};

struct brw_cc_unit_state {
    struct {
	uint pad0:3;
	uint bf_stencil_pass_depth_pass_op:3;
	uint bf_stencil_pass_depth_fail_op:3;
	uint bf_stencil_fail_op:3;
	uint bf_stencil_func:3;
	uint bf_stencil_enable:1;
	uint pad1:2;
	uint stencil_write_enable:1;
	uint stencil_pass_depth_pass_op:3;
	uint stencil_pass_depth_fail_op:3;
	uint stencil_fail_op:3;
	uint stencil_func:3;
	uint stencil_enable:1;
    } cc0;

    struct {
	uint bf_stencil_ref:8;
	uint stencil_write_mask:8;
	uint stencil_test_mask:8;
	uint stencil_ref:8;
    } cc1;

    struct {
	uint logicop_enable:1;
	uint pad0:10;
	uint depth_write_enable:1;
	uint depth_test_function:3;
	uint depth_test:1;
	uint bf_stencil_write_mask:8;
	uint bf_stencil_test_mask:8;
    } cc2;

    struct {
	uint pad0:8;
	uint alpha_test_func:3;
	uint alpha_test:1;
	uint blend_enable:1;
	uint ia_blend_enable:1;
	uint pad1:1;
	uint alpha_test_format:1;
	uint pad2:16;
    } cc3;

    struct {
	uint pad0:5;
	uint cc_viewport_state_offset:27;
    } cc4;

    struct {
	uint pad0:2;
	uint ia_dest_blend_factor:5;
	uint ia_src_blend_factor:5;
	uint ia_blend_function:3;
	uint statistics_enable:1;
	uint logicop_func:4;
	uint pad1:11;
	uint dither_enable:1;
    } cc5;

    struct {
	uint clamp_post_alpha_blend:1;
	uint clamp_pre_alpha_blend:1;
	uint clamp_range:2;
	uint pad0:11;
	uint y_dither_offset:2;
	uint x_dither_offset:2;
	uint dest_blend_factor:5;
	uint src_blend_factor:5;
	uint blend_function:3;
    } cc6;

    struct {
	union {
	    float f;
	    uchar ub[4];
	} alpha_ref;
    } cc7;
};

struct brw_sf_unit_state {
    struct thread0 thread0;
    struct {
	uint pad0:7;
	uint sw_exception_enable:1;
	uint pad1:3;
	uint mask_stack_exception_enable:1;
	uint pad2:1;
	uint illegal_op_exception_enable:1;
	uint pad3:2;
	uint floating_point_mode:1;
	uint thread_priority:1;
	uint binding_table_entry_count:8;
	uint pad4:5;
	uint single_program_flow:1;
    } sf1;

    struct thread2 thread2;
    struct thread3 thread3;

    struct {
	uint pad0:10;
	uint stats_enable:1;
	uint nr_urb_entries:7;
	uint pad1:1;
	uint urb_entry_allocation_size:5;
	uint pad2:1;
	uint max_threads:6;
	uint pad3:1;
    } thread4;

    struct {
	uint front_winding:1;
	uint viewport_transform:1;
	uint pad0:3;
	uint sf_viewport_state_offset:27;
    } sf5;

    struct {
	uint pad0:9;
	uint dest_org_vbias:4;
	uint dest_org_hbias:4;
	uint scissor:1;
	uint disable_2x2_trifilter:1;
	uint disable_zero_pix_trifilter:1;
	uint point_rast_rule:2;
	uint line_endcap_aa_region_width:2;
	uint line_width:4;
	uint fast_scissor_disable:1;
	uint cull_mode:2;
	uint aa_enable:1;
    } sf6;

    struct {
	uint point_size:11;
	uint use_point_size_state:1;
	uint subpixel_precision:1;
	uint sprite_point:1;
	uint pad0:11;
	uint trifan_pv:2;
	uint linestrip_pv:2;
	uint tristrip_pv:2;
	uint line_last_pixel_enable:1;
    } sf7;
};

struct brw_gs_unit_state {
    struct thread0 thread0;
    struct thread1 thread1;
    struct thread2 thread2;
    struct thread3 thread3;

    struct {
	uint pad0:10;
	uint stats_enable:1;
	uint nr_urb_entries:7;
	uint pad1:1;
	uint urb_entry_allocation_size:5;
	uint pad2:1;
	uint max_threads:1;
	uint pad3:6;
    } thread4;

    struct {
	uint sampler_count:3;
	uint pad0:2;
	uint sampler_state_pointer:27;
    } gs5;

    struct {
	uint max_vp_index:4;
	uint pad0:26;
	uint reorder_enable:1;
	uint pad1:1;
    } gs6;
};

struct brw_vs_unit_state {
    struct thread0 thread0;
    struct thread1 thread1;
    struct thread2 thread2;
    struct thread3 thread3;

    struct {
	uint pad0:10;
	uint stats_enable:1;
	uint nr_urb_entries:7;
	uint pad1:1;
	uint urb_entry_allocation_size:5;
	uint pad2:1;
	uint max_threads:4;
	uint pad3:3;
    } thread4;

    struct {
	uint sampler_count:3;
	uint pad0:2;
	uint sampler_state_pointer:27;
    } vs5;

    struct {
	uint vs_enable:1;
	uint vert_cache_disable:1;
	uint pad0:30;
    } vs6;
};

struct brw_wm_unit_state {
    struct thread0 thread0;
    struct thread1 thread1;
    struct thread2 thread2;
    struct thread3 thread3;

    struct {
	uint stats_enable:1;
	uint pad0:1;
	uint sampler_count:3;
	uint sampler_state_pointer:27;
    } wm4;

    struct {
	uint enable_8_pix:1;
	uint enable_16_pix:1;
	uint enable_32_pix:1;
	uint pad0:7;
	uint legacy_global_depth_bias:1;
	uint line_stipple:1;
	uint depth_offset:1;
	uint polygon_stipple:1;
	uint line_aa_region_width:2;
	uint line_endcap_aa_region_width:2;
	uint early_depth_test:1;
	uint thread_dispatch_enable:1;
	uint program_uses_depth:1;
	uint program_computes_depth:1;
	uint program_uses_killpixel:1;
	uint legacy_line_rast: 1;
	uint transposed_urb_read:1;
	uint max_threads:7;
    } wm5;

    float global_depth_offset_constant;
    float global_depth_offset_scale;
};

/* The hardware supports two different modes for border color. The
 * default (OpenGL) mode uses floating-point color channels, while the
 * legacy mode uses 4 bytes.
 *
 * More significantly, the legacy mode respects the components of the
 * border color for channels not present in the source, (whereas the
 * default mode will ignore the border color's alpha channel and use
 * alpha==1 for an RGB source, for example).
 *
 * The legacy mode matches the semantics specified by the Render
 * extension.
 */
struct brw_sampler_default_border_color {
    float color[4];
};

struct brw_sampler_legacy_border_color {
    uint8_t color[4];
};

struct brw_sampler_state {
    struct {
	uint shadow_function:3;
	uint lod_bias:11;
	uint min_filter:3;
	uint mag_filter:3;
	uint mip_filter:2;
	uint base_level:5;
	uint pad:1;
	uint lod_preclamp:1;
	uint border_color_mode:1;
	uint pad0:1;
	uint disable:1;
    } ss0;

    struct {
	uint r_wrap_mode:3;
	uint t_wrap_mode:3;
	uint s_wrap_mode:3;
	uint pad:3;
	uint max_lod:10;
	uint min_lod:10;
    } ss1;

    struct {
	uint pad:5;
	uint border_color_pointer:27;
    } ss2;

    struct {
	uint pad:19;
	uint max_aniso:3;
	uint chroma_key_mode:1;
	uint chroma_key_index:2;
	uint chroma_key_enable:1;
	uint monochrome_filter_width:3;
	uint monochrome_filter_height:3;
    } ss3;
};

struct brw_clipper_viewport {
    float xmin;
    float xmax;
    float ymin;
    float ymax;
};

struct brw_cc_viewport {
    float min_depth;
    float max_depth;
};

struct brw_sf_viewport {
    struct {
	float m00;
	float m11;
	float m22;
	float m30;
	float m31;
	float m32;
    } viewport;

    struct {
	short xmin;
	short ymin;
	short xmax;
	short ymax;
    } scissor;
};

/* Documented in the subsystem/shared-functions/sampler chapter...
*/
struct brw_surface_state {
    struct {
	uint cube_pos_z:1;
	uint cube_neg_z:1;
	uint cube_pos_y:1;
	uint cube_neg_y:1;
	uint cube_pos_x:1;
	uint cube_neg_x:1;
	uint pad:3;
	uint render_cache_read_mode:1;
	uint mipmap_layout_mode:1;
	uint vert_line_stride_ofs:1;
	uint vert_line_stride:1;
	uint color_blend:1;
	uint writedisable_blue:1;
	uint writedisable_green:1;
	uint writedisable_red:1;
	uint writedisable_alpha:1;
	uint surface_format:9;
	uint data_return_format:1;
	uint pad0:1;
	uint surface_type:3;
    } ss0;

    struct {
	uint base_addr;
    } ss1;

    struct {
	uint render_target_rotation:2;
	uint mip_count:4;
	uint width:13;
	uint height:13;
    } ss2;

    struct {
	uint tile_walk:1;
	uint tiled_surface:1;
	uint pad:1;
	uint pitch:18;
	uint depth:11;
    } ss3;

    struct {
	uint pad:19;
	uint min_array_elt:9;
	uint min_lod:4;
    } ss4;

    struct {
	uint pad:20;
	uint y_offset:4;
	uint pad2:1;
	uint x_offset:7;
    } ss5;
};

struct brw_vertex_buffer_state {
    struct {
	uint pitch:11;
	uint pad:15;
	uint access_type:1;
	uint vb_index:5;
    } vb0;

    uint start_addr;
    uint max_index;
#if 1
    uint instance_data_step_rate; /* not included for sequential/random vertices? */
#endif
};

#define BRW_VBP_MAX 17

struct brw_vb_array_state {
    struct header header;
    struct brw_vertex_buffer_state vb[BRW_VBP_MAX];
};

struct brw_vertex_element_state {
    struct {
	uint src_offset:11;
	uint pad:5;
	uint src_format:9;
	uint pad0:1;
	uint valid:1;
	uint vertex_buffer_index:5;
    } ve0;

    struct {
	uint dst_offset:8;
	uint pad:8;
	uint vfcomponent3:4;
	uint vfcomponent2:4;
	uint vfcomponent1:4;
	uint vfcomponent0:4;
    } ve1;
};

#define BRW_VEP_MAX 18

struct brw_vertex_element_packet {
    struct header header;
    struct brw_vertex_element_state ve[BRW_VEP_MAX];
};

struct brw_urb_immediate {
    uint opcode:4;
    uint offset:6;
    uint swizzle_control:2;
    uint pad:1;
    uint allocate:1;
    uint used:1;
    uint complete:1;
    uint response_length:4;
    uint msg_length:4;
    uint msg_target:4;
    uint pad1:3;
    uint end_of_thread:1;
};

/* Instruction format for the execution units: */

struct brw_instruction {
    struct {
	uint opcode:7;
	uint pad:1;
	uint access_mode:1;
	uint mask_control:1;
	uint dependency_control:2;
	uint compression_control:2;
	uint thread_control:2;
	uint predicate_control:4;
	uint predicate_inverse:1;
	uint execution_size:3;
	uint destreg__conditonalmod:4; /* destreg - send, conditionalmod - others */
	uint pad0:2;
	uint debug_control:1;
	uint saturate:1;
    } header;

    union {
	struct {
	    uint dest_reg_file:2;
	    uint dest_reg_type:3;
	    uint src0_reg_file:2;
	    uint src0_reg_type:3;
	    uint src1_reg_file:2;
	    uint src1_reg_type:3;
	    uint pad:1;
	    uint dest_subreg_nr:5;
	    uint dest_reg_nr:8;
	    uint dest_horiz_stride:2;
	    uint dest_address_mode:1;
	} da1;

	struct {
	    uint dest_reg_file:2;
	    uint dest_reg_type:3;
	    uint src0_reg_file:2;
	    uint src0_reg_type:3;
	    uint pad:6;
	    int dest_indirect_offset:10;	/* offset against the deref'd address reg */
	    uint dest_subreg_nr:3; /* subnr for the address reg a0.x */
	    uint dest_horiz_stride:2;
	    uint dest_address_mode:1;
	} ia1;

	struct {
	    uint dest_reg_file:2;
	    uint dest_reg_type:3;
	    uint src0_reg_file:2;
	    uint src0_reg_type:3;
	    uint src1_reg_file:2;
	    uint src1_reg_type:3;
	    uint pad0:1;
	    uint dest_writemask:4;
	    uint dest_subreg_nr:1;
	    uint dest_reg_nr:8;
	    uint pad1:2;
	    uint dest_address_mode:1;
	} da16;

	struct {
	    uint dest_reg_file:2;
	    uint dest_reg_type:3;
	    uint src0_reg_file:2;
	    uint src0_reg_type:3;
	    uint pad0:6;
	    uint dest_writemask:4;
	    int dest_indirect_offset:6;
	    uint dest_subreg_nr:3;
	    uint pad1:2;
	    uint dest_address_mode:1;
	} ia16;
    } bits1;


    union {
	struct {
	    uint src0_subreg_nr:5;
	    uint src0_reg_nr:8;
	    uint src0_abs:1;
	    uint src0_negate:1;
	    uint src0_address_mode:1;
	    uint src0_horiz_stride:2;
	    uint src0_width:3;
	    uint src0_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad:6;
	} da1;

	struct {
	    int src0_indirect_offset:10;
	    uint src0_subreg_nr:3;
	    uint src0_abs:1;
	    uint src0_negate:1;
	    uint src0_address_mode:1;
	    uint src0_horiz_stride:2;
	    uint src0_width:3;
	    uint src0_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad:6;
	} ia1;

	struct {
	    uint src0_swz_x:2;
	    uint src0_swz_y:2;
	    uint src0_subreg_nr:1;
	    uint src0_reg_nr:8;
	    uint src0_abs:1;
	    uint src0_negate:1;
	    uint src0_address_mode:1;
	    uint src0_swz_z:2;
	    uint src0_swz_w:2;
	    uint pad0:1;
	    uint src0_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad1:6;
	} da16;

	struct {
	    uint src0_swz_x:2;
	    uint src0_swz_y:2;
	    int src0_indirect_offset:6;
	    uint src0_subreg_nr:3;
	    uint src0_abs:1;
	    uint src0_negate:1;
	    uint src0_address_mode:1;
	    uint src0_swz_z:2;
	    uint src0_swz_w:2;
	    uint pad0:1;
	    uint src0_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad1:6;
	} ia16;

    } bits2;

    union {
	struct {
	    uint src1_subreg_nr:5;
	    uint src1_reg_nr:8;
	    uint src1_abs:1;
	    uint src1_negate:1;
	    uint pad:1;
	    uint src1_horiz_stride:2;
	    uint src1_width:3;
	    uint src1_vert_stride:4;
	    uint pad0:7;
	} da1;

	struct {
	    uint src1_swz_x:2;
	    uint src1_swz_y:2;
	    uint src1_subreg_nr:1;
	    uint src1_reg_nr:8;
	    uint src1_abs:1;
	    uint src1_negate:1;
	    uint pad0:1;
	    uint src1_swz_z:2;
	    uint src1_swz_w:2;
	    uint pad1:1;
	    uint src1_vert_stride:4;
	    uint pad2:7;
	} da16;

	struct {
	    int src1_indirect_offset:10;
	    uint src1_subreg_nr:3;
	    uint src1_abs:1;
	    uint src1_negate:1;
	    uint pad0:1;
	    uint src1_horiz_stride:2;
	    uint src1_width:3;
	    uint src1_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad1:6;
	} ia1;

	struct {
	    uint src1_swz_x:2;
	    uint src1_swz_y:2;
	    int src1_indirect_offset:6;
	    uint src1_subreg_nr:3;
	    uint src1_abs:1;
	    uint src1_negate:1;
	    uint pad0:1;
	    uint src1_swz_z:2;
	    uint src1_swz_w:2;
	    uint pad1:1;
	    uint src1_vert_stride:4;
	    uint flag_reg_nr:1;
	    uint pad2:6;
	} ia16;

	struct {
	    int jump_count:16;	/* note: signed */
	    uint pop_count:4;
	    uint pad0:12;
	} if_else;

	struct {
	    uint function:4;
	    uint int_type:1;
	    uint precision:1;
	    uint saturate:1;
	    uint data_type:1;
	    uint pad0:8;
	    uint response_length:4;
	    uint msg_length:4;
	    uint msg_target:4;
	    uint pad1:3;
	    uint end_of_thread:1;
	} math;

	struct {
	    uint binding_table_index:8;
	    uint sampler:4;
	    uint return_format:2;
	    uint msg_type:2;
	    uint response_length:4;
	    uint msg_length:4;
	    uint msg_target:4;
	    uint pad1:3;
	    uint end_of_thread:1;
	} sampler;

	struct {
	    uint32_t binding_table_index:8;
	    uint32_t sampler:4;
	    uint32_t msg_type:4;
	    uint32_t response_length:4;
	    uint32_t msg_length:4;
	    uint32_t msg_target:4;
	    uint32_t pad1:3;
	    uint32_t end_of_thread:1;
	} sampler_g4x;

	struct brw_urb_immediate urb;

	struct {
	    uint binding_table_index:8;
	    uint msg_control:4;
	    uint msg_type:2;
	    uint target_cache:2;
	    uint response_length:4;
	    uint msg_length:4;
	    uint msg_target:4;
	    uint pad1:3;
	    uint end_of_thread:1;
	} dp_read;

	struct {
	    uint binding_table_index:8;
	    uint msg_control:3;
	    uint pixel_scoreboard_clear:1;
	    uint msg_type:3;
	    uint send_commit_msg:1;
	    uint response_length:4;
	    uint msg_length:4;
	    uint msg_target:4;
	    uint pad1:3;
	    uint end_of_thread:1;
	} dp_write;

	struct {
	    uint pad:16;
	    uint response_length:4;
	    uint msg_length:4;
	    uint msg_target:4;
	    uint pad1:3;
	    uint end_of_thread:1;
	} generic;

	uint32_t ud;
	int32_t d;
    } bits3;
};

/* media pipeline */

struct brw_vfe_state {
    struct {
	uint per_thread_scratch_space:4;
	uint pad3:3;
	uint extend_vfe_state_present:1;
	uint pad2:2;
	uint scratch_base:22;
    } vfe0;

    struct {
	uint debug_counter_control:2;
	uint children_present:1;
	uint vfe_mode:4;
	uint pad2:2;
	uint num_urb_entries:7;
	uint urb_entry_alloc_size:9;
	uint max_threads:7;
    } vfe1;

    struct {
	uint pad4:4;
	uint interface_descriptor_base:28;
    } vfe2;
};

struct brw_vld_state {
    struct {
	uint pad6:6;
	uint scan_order:1;
	uint intra_vlc_format:1;
	uint quantizer_scale_type:1;
	uint concealment_motion_vector:1;
	uint frame_predict_frame_dct:1;
	uint top_field_first:1;
	uint picture_structure:2;
	uint intra_dc_precision:2;
	uint f_code_0_0:4;
	uint f_code_0_1:4;
	uint f_code_1_0:4;
	uint f_code_1_1:4;
    } vld0;

    struct {
	uint pad2:9;
	uint picture_coding_type:2;
	uint pad:21;
    } vld1;

    struct {
	uint index_0:4;
	uint index_1:4;
	uint index_2:4;
	uint index_3:4;
	uint index_4:4;
	uint index_5:4;
	uint index_6:4;
	uint index_7:4;
    } desc_remap_table0;

    struct {
	uint index_8:4;
	uint index_9:4;
	uint index_10:4;
	uint index_11:4;
	uint index_12:4;
	uint index_13:4;
	uint index_14:4;
	uint index_15:4;
    } desc_remap_table1;
};

struct brw_interface_descriptor {
    struct {
	uint grf_reg_blocks:4;
	uint pad:2;
	uint kernel_start_pointer:26;
    } desc0;

    struct {
	uint pad:7;
	uint software_exception:1;
	uint pad2:3;
	uint maskstack_exception:1;
	uint pad3:1;
	uint illegal_opcode_exception:1;
	uint pad4:2;
	uint floating_point_mode:1;
	uint thread_priority:1;
	uint single_program_flow:1;
	uint pad5:1;
	uint const_urb_entry_read_offset:6;
	uint const_urb_entry_read_len:6;
    } desc1;

    struct {
	uint pad:2;
	uint sampler_count:3;
	uint sampler_state_pointer:27;
    } desc2;

    struct {
	uint binding_table_entry_count:5;
	uint binding_table_pointer:27;
    } desc3;
};

#endif
