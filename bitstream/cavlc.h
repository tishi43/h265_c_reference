#ifndef __CAVLC_H__
#define __CAVLC_H__

#define FIX827 1 ///< Fix for issue #827: CABAC init tables
#define FIX712 1 ///< Fix for issue #712: CABAC init tables

#define CNU                          154      ///< dummy initialization value for unused context models 'Context model Not Used'


struct context_model {
	int valMps;
	int pStateIdx;
};

void init_context_variables(struct context_model *cm, int SliceQpY, int initValue);
void parse_sao_merge(unsigned char *rbsp, int *synVal);
void parse_sao_type_idx(unsigned char *rbsp, int *synVal);
void parse_sao_tr(unsigned char *rbsp, int *synVal, int cMax);
void init_cabac(unsigned char *rbsp, int SliceQpY,
	struct pic_parameter_set *pps, struct slice_segment_header *slice_head);
void decode_bin(unsigned char *rbsp, int *binVal, struct context_model *cm);
void bypass_decode_bin(unsigned char *rbsp, int *binVal);
void bypass_decode_bins(unsigned char *rbsp, int *synVal, int fixedLength);
void terminate_decode_bin(unsigned char *rbsp, int *synVal);

void parse_split_cu_flag(unsigned char *rbsp, int *split_cu_flag, int x0, int y0, int log2CbSize, int cqtDepth);
void parse_cu_transquant_bypass_flag(unsigned char *rbsp, int *synVal);
void parse_cu_skip_flag(unsigned char *rbsp, int x0, int y0, int log2CbSize);
void parse_pred_mode_flag(unsigned char *rbsp, int *synVal, int x0, int y0, int log2CbSize);
void parse_part_mode(unsigned char *rbsp, int *synVal, int x0, int y0, int log2CbSize,
	struct seq_parameter_set *sps);
void parse_intra_chroma_pred_mode(unsigned char *rbsp, int x0, int y0, int *IntraPredModeC, int IntraPredModeY);
void parse_merge_idx(unsigned char *rbsp, int *synVal, int MaxNumMergeCand);
void parse_merge_flag(unsigned char *rbsp, int *synVal);
void parse_rqt_root_cbf(unsigned char *rbsp, int *synVal);
void parse_ref_idx_l0(unsigned char *rbsp, int *synVal, int num_ref_idx_l0_active_minus1);
void parse_mvp_l0_flag(unsigned char *rbsp, int *synVal);
void parse_mvd_coding(unsigned char *rbsp, struct Mv *mvd);
void parse_abs_mvd_minus2(unsigned char *rbsp, int *synVal);
void parse_split_transform_flag(unsigned char *rbsp, int *synVal, int log2TrafoSize);
void parse_cbf_cb_cr(unsigned char *rbsp, int *synVal, int trafoDepth);
void parse_cbf_luma(unsigned char *rbsp, int *synVal, int trafoDepth);
void parse_cu_qp_delta_abs(unsigned char *rbsp, int *synVal);
void parse_transform_skip_flag(unsigned char *rbsp, int *synVal, int cIdx);
void parse_last_sig_coeff_x_prefix(unsigned char *rbsp, int *synVal, int log2TrafoSize, int cIdx);
void parse_last_sig_coeff_y_prefix(unsigned char *rbsp, int *synVal, int log2TrafoSize, int cIdx);
void parse_last_sig_coeff_suffix(unsigned char *rbsp, int *synVal, int last_sig_coeff_prefix);
void parse_coded_sub_block_flag(unsigned char *rbsp, int *synVal, int cIdx, int csbfCtx);
void parse_sig_coeff_flag(unsigned char *rbsp, int *synVal, int cIdx,
	int prevCsbf, int xC, int yC, int xS, int yS, int scanIdx, int log2TrafoSize);
void parse_coeff_abs_level_greater1_flag(unsigned char *rbsp, int *synVal, int cIdx, int ctxInc);
void parse_coeff_abs_level_greater2_flag(unsigned char *rbsp, int *synVal, int cIdx, int ctxInc);
void parse_coeff_abs_level_remaining(unsigned char *rbsp, int *synVal, int cRiceParam);
void parse_prev_intra_luma_pred_flag(unsigned char *rbsp, int *synVal);
void parse_mpm_idx(unsigned char *rbsp, int *synVal);







#endif