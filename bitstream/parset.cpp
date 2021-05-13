#include "stdafx.h"
#include "bitstream_header.h"
#include <assert.h>

void parse_profile_tier(unsigned char *rbsp, struct video_parameter_set *vps)
{
	int val;
	int j;
	vps->general_profile_space = read_bits(rbsp, 2);
	vps->general_tier_flag  = read_one_bit(rbsp);
	vps->general_profile_idc = read_bits(rbsp, 5);
	for (j = 0;j < 32; j++) {
		vps->general_profile_compatibility_j[j] = read_one_bit(rbsp);
	}
	vps->general_progressive_source_flag = read_one_bit(rbsp);;
	vps->general_interlaced_source_flag = read_one_bit(rbsp);
	vps->general_non_packed_constraint_flag = read_one_bit(rbsp);
	vps->general_frame_only_constraint_flag = read_one_bit(rbsp);
	val = read_bits(rbsp, 32); // general_reserved_zero_44bits
	assert(val == 0);
	val = read_bits(rbsp, 12); // general_reserved_zero_44bits
	assert(val == 0);

	vps->general_level_idc = read_bits(rbsp, 8);

}

void parse_short_term_ref_pic_set(unsigned char *rbsp, struct reference_picture_set *rps, 
					int stRpsIdx,
					struct seq_parameter_set *sps)
{
	int val;
	int i, j;
	int RefRpsIdx;
	int num_negative_pics, num_positive_pics;
	int delta_poc_s0_minus1;
	int delta_rps_sign, abs_delta_rps_minus1, deltaRps;
	int used_by_curr_pic_flag[16], use_delta_flag[16];
	int prev, poc;
	if (stRpsIdx != 0)
		rps->inter_ref_pic_set_prediction_flag = read_one_bit(rbsp);
	else
		rps->inter_ref_pic_set_prediction_flag = 0;

	if (rps->inter_ref_pic_set_prediction_flag) {
		if (stRpsIdx == sps->num_short_term_ref_pic_sets) { //call from parse slice_header
			val = read_ue_v(rbsp);
			rps->delta_idx_minus1 = val;
		} else {
			val = 0;
		}
//所谓RefRpsIdx，也就是当前inter rps参考哪个index的rps，一般是参考stRpsIdx-1,也就是当前rps前一个rps
		RefRpsIdx = stRpsIdx - val - 1;
		assert(val <= stRpsIdx - 1); // delta_idx_minus1 shall not be larger than idx-1, otherwise we will predict from a negative row position that does not exist. When idx equals 0 there is no legal value and interRPSPred must be zero. See J0185-r2
		assert (RefRpsIdx <= stRpsIdx - 1 && RefRpsIdx >= 0);
		delta_rps_sign = read_one_bit(rbsp);
		abs_delta_rps_minus1 = read_ue_v(rbsp);
		deltaRps = (1 - 2 * delta_rps_sign) * (abs_delta_rps_minus1 + 1);
		sps->rps[stRpsIdx].numberOfPictures = 0;
		for (j = 0; j <= sps->rps[RefRpsIdx].numberOfPictures; j++) {
			used_by_curr_pic_flag[j] = read_one_bit(rbsp);
			if (used_by_curr_pic_flag[j] == 0) {
				use_delta_flag[j] = read_one_bit(rbsp);
			} else
				use_delta_flag[j] = 1;
		}
		i = 0;
		if (deltaRps < 0 && use_delta_flag[sps->rps[RefRpsIdx].numberOfPictures]) {
			sps->rps[stRpsIdx].deltaPoc[i] = deltaRps;
			sps->rps[stRpsIdx].usedByCurr[i++] = used_by_curr_pic_flag[sps->rps[RefRpsIdx].numberOfPictures];
		}
		for (j = 0; j < sps->rps[RefRpsIdx].numberOfPictures; j++) {
			int dPoc = sps->rps[RefRpsIdx].deltaPoc[j] + deltaRps;
			if (dPoc < 0 &&  use_delta_flag[j]) {

				sps->rps[stRpsIdx].deltaPoc[i] = dPoc;
				sps->rps[stRpsIdx].usedByCurr[i++] = used_by_curr_pic_flag[j];
			}
		}
		sps->rps[stRpsIdx].numberOfPictures = i;
	} else {
		num_negative_pics = read_ue_v(rbsp);
		num_positive_pics = read_ue_v(rbsp);
		assert(num_positive_pics == 0);

		sps->rps[stRpsIdx].numberOfPictures = num_negative_pics;
		prev = 0;
		for (j = 0; j < num_negative_pics; j++) {
			delta_poc_s0_minus1 = read_ue_v(rbsp);
			used_by_curr_pic_flag[j] = read_one_bit(rbsp);
			poc = prev - delta_poc_s0_minus1 - 1;
			prev = poc;
			sps->rps[stRpsIdx].deltaPoc[j] = poc;
			sps->rps[stRpsIdx].usedByCurr[j] = used_by_curr_pic_flag[j];
		}
	}
}


void parse_sps(unsigned char *rbsp, struct seq_parameter_set *sps, struct video_parameter_set *vps)
{
	int i;
	sps->sps_video_parameter_set_id = read_bits(rbsp, 4);
	sps->sps_max_sub_layers_minus1 = read_bits(rbsp, 3);
	assert(sps->sps_max_sub_layers_minus1 <= 6);
	sps->sps_temporal_id_nesting_flag = read_one_bit(rbsp);
	assert(sps->sps_temporal_id_nesting_flag == 1);
	parse_profile_tier(rbsp, vps);
	sps->sps_seq_parameter_set_id = read_ue_v(rbsp);
	assert(sps->sps_seq_parameter_set_id <= 15);
	sps->chroma_format_idc = read_ue_v(rbsp);
	// in the first version we only support chroma_format_idc equal to 1 (4:2:0), 
	//so separate_colour_plane_flag cannot appear in the bitstream
	assert(sps->chroma_format_idc == 1);
	sps->pic_width_in_luma_samples = read_ue_v(rbsp);
	sps->pic_height_in_luma_samples = read_ue_v(rbsp);
	sps->conformance_window_flag = read_one_bit(rbsp);
	if (sps->conformance_window_flag == 1) {
		sps->conf_win_left_offset = read_ue_v(rbsp);
		sps->conf_win_right_offset = read_ue_v(rbsp);
		sps->conf_win_top_offset = read_ue_v(rbsp);
		sps->conf_win_bottom_offset = read_ue_v(rbsp);
	}
	sps->bit_depth_luma_minus8 = read_ue_v(rbsp);
	assert(sps->bit_depth_luma_minus8 <=6);
	sps->bit_depth_chroma_minus8 = read_ue_v(rbsp);
	assert(sps->bit_depth_chroma_minus8 <=6);
	sps->log2_max_pic_order_cnt_lsb_minus4 = read_ue_v(rbsp); //=4  2^(4+4)=256 POC最大256
	assert(sps->log2_max_pic_order_cnt_lsb_minus4 <= 12);
	sps->sps_sub_layer_ordering_info_present_flag = read_one_bit(rbsp);
	
	//assert sps_max_sub_layers_minus1=0
	sps->sps_max_dec_pic_buffering_minus1 = read_ue_v(rbsp); //参考帧数，spec里，这个必须小于等于vps的那个
	sps->sps_num_reorder_pics = read_ue_v(rbsp);
	sps->sps_max_latency_increase_plus1 = read_ue_v(rbsp);
	sps->log2_min_coding_block_size_minus3 = read_ue_v(rbsp); //=0 CU最小2^3=8x8
	sps->log2_diff_max_min_coding_block_size = read_ue_v(rbsp); //=3 最大最小CU相差3，最大2^(3+3)=64
	sps->log2_min_transform_block_size_minus2 = read_ue_v(rbsp); //=0 TU最小 2^2 = 4x4
	sps->log2_diff_max_min_transform_block_size = read_ue_v(rbsp); //=3 最大最小TU相差3，最大2^(2+3)=32
	sps->max_transform_hierarchy_depth_inter = read_ue_v(rbsp); //=2 TU内部depth最大2层
	sps->max_transform_hierarchy_depth_intra = read_ue_v(rbsp);
	sps->scaling_list_enabled_flag = read_one_bit(rbsp);
	assert(sps->scaling_list_enabled_flag == 0);
	sps->amp_enabled_flag = read_one_bit(rbsp); //=1
	sps->sample_adaptive_offset_enabled_flag = read_one_bit(rbsp); //=1
	sps->pcm_enabled_flag = read_one_bit(rbsp); //=0
	assert(sps->pcm_enabled_flag == 0);
	sps->num_short_term_ref_pic_sets = read_ue_v(rbsp);
	assert(sps->num_short_term_ref_pic_sets <= 64); //=13
	for (i = 0; i < sps->num_short_term_ref_pic_sets; i++) {
		parse_short_term_ref_pic_set(rbsp, &sps->rps[i], i, sps);
	}
	sps->long_term_ref_pics_present_flag = read_one_bit(rbsp);
	assert(sps->long_term_ref_pics_present_flag == 0);
	sps->sps_temporal_mvp_enabled_flag = read_one_bit(rbsp);
	sps->strong_intra_smoothing_enabled_flag = read_one_bit(rbsp);
	sps->vui_parameters_present_flag = read_one_bit(rbsp);
	assert(sps->vui_parameters_present_flag == 0);
	sps->sps_extension_flag = read_one_bit(rbsp);
	assert(sps->sps_extension_flag == 0);

}


void parse_vps(unsigned char *rbsp, struct video_parameter_set *vps)
{
	int val;
	vps->vps_video_parameter_set_id = read_bits(rbsp, 4);
	val = read_bits(rbsp, 2); //vps_reserved_three_2bits

	val = read_bits(rbsp, 6); //vps_max_layers_minus1
	assert(val == 0);
	vps->vps_max_sub_layers_minus1 = read_bits(rbsp, 3);
	assert(vps->vps_max_sub_layers_minus1 == 0);
	vps->vps_temporal_id_nesting_flag = read_one_bit(rbsp);
	val = read_bits(rbsp, 16);
	assert(val == 0xffff); //vps_reserved_ffff_16bits
	parse_profile_tier(rbsp, vps);
	vps->vps_sub_layer_ordering_info_present_flag = read_one_bit(rbsp);
	vps->vps_max_dec_pic_buffering_minus1_i = read_ue_v(rbsp); //=4 也就是参考帧5帧
	vps->vps_max_num_reorder_pics_i = read_ue_v(rbsp);
	vps->vps_max_latency_increase_plus1_i = read_ue_v(rbsp);
	vps->vps_max_layer_id = read_bits(rbsp, 6); //vps_max_nuh_reserved_zero_layer_id
	vps->vps_num_layer_sets_minus1 = read_ue_v(rbsp);//vps_max_op_sets_minus1
	assert(vps->vps_num_layer_sets_minus1 == 0);
	vps->vps_timing_info_present_flag = read_one_bit(rbsp);
	assert(vps->vps_timing_info_present_flag == 0);
	vps->vps_extension_flag = read_one_bit(rbsp);
	assert(vps->vps_extension_flag == 0);
}

void parse_pps(unsigned char *rbsp, struct pic_parameter_set *pps)
{
	pps->pps_pic_parameter_set_id = read_ue_v(rbsp);
	assert(pps->pps_pic_parameter_set_id <= 63);
	pps->pps_seq_parameter_set_id = read_ue_v(rbsp);
	assert(pps->pps_seq_parameter_set_id <= 15);
	pps->dependent_slice_segments_enabled_flag = read_one_bit(rbsp); //=0
	//标识slice header里是否存在dependent_slice_segment_flag
	assert(pps->dependent_slice_segments_enabled_flag == 0);
	pps->output_flag_present_flag = read_one_bit(rbsp); //=0
	//标识slice header里是否存在pic_output_flag
	pps->num_extra_slice_header_bits = read_bits(rbsp, 3);
	assert(pps->num_extra_slice_header_bits == 0);
	pps->sign_data_hiding_enabled_flag = read_one_bit(rbsp);
	pps->cabac_init_present_flag = read_one_bit(rbsp);
	pps->num_ref_idx_l0_default_active_minus1 = read_ue_v(rbsp);
	assert(pps->num_ref_idx_l0_default_active_minus1 <= 15);
	pps->num_ref_idx_l1_default_active_minus1 = read_ue_v(rbsp);
	assert(pps->num_ref_idx_l0_default_active_minus1 <= 15);
	pps->init_qp_minus26 = read_se_v(rbsp);
	pps->constrained_intra_pred_flag = read_one_bit(rbsp);
	pps->transform_skip_enabled_flag = read_one_bit(rbsp);
	pps->cu_qp_delta_enabled_flag = read_one_bit(rbsp);
	if (pps->cu_qp_delta_enabled_flag) {
		pps->diff_cu_qp_delta_depth = read_ue_v(rbsp);
	}
	else {
		pps->diff_cu_qp_delta_depth = 0;
	}
	pps->pps_cb_qp_offset = read_se_v(rbsp);
	assert(pps->pps_cb_qp_offset >= -12);
	assert(pps->pps_cb_qp_offset <= 12);
	pps->pps_cr_qp_offset = read_se_v(rbsp);
	assert(pps->pps_cr_qp_offset >= -12);
	assert(pps->pps_cr_qp_offset <= 12);
	pps->pps_slice_chroma_qp_offsets_present_flag = read_one_bit(rbsp);
	pps->weighted_pred_flag = read_one_bit(rbsp);
	assert(pps->weighted_pred_flag == 0);
	pps->weighted_pred_flag = read_one_bit(rbsp);
	assert(pps->weighted_pred_flag == 0);
	pps->transquant_bypass_enabled_flag = read_one_bit(rbsp); //=0
	//表示是否跳过scaling和变换过程,如果变换、量化过程被旁路,则直接将残差pcResidual赋值给rpcCoeff 
	pps->tiles_enabled_flag = read_one_bit(rbsp);
	assert(pps->tiles_enabled_flag == 0);
	pps->entropy_coding_sync_enabled_flag = read_one_bit(rbsp);
	assert(pps->entropy_coding_sync_enabled_flag == 0); //todo
	pps->loop_filter_across_slices_enabled_flag = read_one_bit(rbsp);
	pps->deblocking_filter_control_present_flag = read_one_bit(rbsp);
	if (pps->deblocking_filter_control_present_flag) {
		pps->deblocking_filter_override_enabled_flag = read_one_bit(rbsp);
		pps->pps_deblocking_filter_disabled_flag = read_one_bit(rbsp);
		if (!pps->pps_deblocking_filter_disabled_flag) {
			pps->pps_beta_offset_div2 = read_se_v(rbsp);
			pps->pps_tc_offset_div2 = read_se_v(rbsp);
		}
	} else {
		pps->pps_deblocking_filter_disabled_flag = 0;
		pps->pps_beta_offset_div2 = 0;
		pps->pps_tc_offset_div2 = 0;
		//When not present, the value of pps_deblocking_filter_disabled_flag is  inferred to be equal to 0.
		//When not present, the value of pps_beta_offset_div2 and pps_tc_offset_div2 are inferred to be equal to 0.
	}
	pps->pps_scaling_list_data_present_flag = read_one_bit(rbsp);
	assert(pps->pps_scaling_list_data_present_flag == 0);
	pps->lists_modification_present_flag = read_one_bit(rbsp);
	assert(pps->lists_modification_present_flag == 0);
	pps->log2_parallel_merge_level_minus2 = read_se_v(rbsp);
	pps->slice_segment_header_extension_present_flag = read_one_bit(rbsp);
	pps->pps_extension_flag = read_one_bit(rbsp);
	assert(pps->pps_extension_flag == 0);
}

