// bitstream.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <malloc.h>

#include "bitstream_header.h"

int line_buf[516]; //(129)*4 TU最大32x32的话，实际line_buf 65*4足够，后129*2用于smooth filtered之后的samples
extern struct HEVCFrame *CurrPic;
extern int PicWidthInSamplesY, PicHeightInSamplesY;
extern int Log2MinTrafoSize;
extern int MinCbLog2SizeY;
extern int MinCbSizeY;
extern int **predSamplesL, **predSamplesCb, **predSamplesCr;
extern int slice_num;

//xTbY,yTbY,亮度TB相对于图像坐标
//xTbCmp, yTbCmp,亮度，色度TB相对于亮度色度图像块的坐标
void intra_pred(int xTbY, int yTbY, int cIdx, int log2TrafoSize, int predModeIntra,
	struct pic_parameter_set *pps,
	struct seq_parameter_set *sps)
{
	int left_avail[17] = {0,};
	//左上,左边和左下的avail，以8x8(MinCbSizeY)为单位，亮度左+左下最大128，16份,再加一个左上
	//不可能一个8x8的Cb，上面8x4是intra，下面8x4是inter
	int up_avail[16] = {0,};
	int nTbS = 1 << log2TrafoSize;
	int *p_left = line_buf + 1;
	int *p_top = line_buf + 130;
	int **predSamples;
	unsigned char **src;
	int xTbCmp = xTbY, yTbCmp = yTbY;
	int i, j;
	int nTbS_temp = nTbS;
	int size_in_mincbs;
	int unit_size = (cIdx == 0) ? MinCbSizeY : (MinCbSizeY >> 1);
	int all_unavail = 1;
	int found = 0;
	Log_data("intrapred xTbY %d yTbY %d cIdx %d log2TrafoSize %d\n", xTbY, yTbY, cIdx, log2TrafoSize);


	if (cIdx != 0) {
		xTbCmp = xTbCmp >> 1;
		yTbCmp = yTbCmp >> 1;
		nTbS_temp = nTbS * 2; //色度最大32x32，nxn的色度求avail时要以2nx2n的亮度块来求
	}
	if (nTbS_temp >= (1 << MinCbLog2SizeY)) {
		size_in_mincbs = nTbS_temp >> MinCbLog2SizeY;
	} else {
		size_in_mincbs = 1;
		unit_size = nTbS;
	}
	if (cIdx == 0) {
		predSamples = predSamplesL;
		src = CurrPic->SL;
	}
	else if (cIdx == 1) {
		predSamples = predSamplesCb;
		src = CurrPic->SCb;
	}
	else {
		predSamples = predSamplesCr;
		src = CurrPic->SCr;
	}

	//left up
	if (xTbY > 0 && yTbY > 0) {
		if (pps->constrained_intra_pred_flag == 0 ||
			CurrPic->cu_predmode[(yTbY - 1) >> MinCbLog2SizeY][(xTbY - 1) >> MinCbLog2SizeY] == MODE_INTRA) {
				left_avail[0] = 1;
				all_unavail = 0;
				p_left[-1] = src[yTbCmp-1][xTbCmp-1];
				p_top[-1] = p_left[-1];
		}
	}
	//left
	for (i = 0; i < size_in_mincbs; i++) {
		if (xTbY > 0) {
			if (pps->constrained_intra_pred_flag == 0 ||
				CurrPic->cu_predmode[(yTbY + i*MinCbSizeY) >> MinCbLog2SizeY][(xTbY - 1) >> MinCbLog2SizeY] == MODE_INTRA) {
				left_avail[i + 1] = 1;
				all_unavail = 0;
				for (j = 0; j < unit_size; j++)
					p_left[i*unit_size+j] = src[yTbCmp+i*unit_size+j][xTbCmp-1];
			}
		}
	}

	//left bottom 4的左下3可用
	// 0  |  1 |  4 |  5 |
	//--------------------
	// 2  |  3 |  6 |  7 |
	//----|---------|----
	for (i = 0; i < size_in_mincbs; i++) {
		if (test_block_available(xTbY, yTbY+nTbS_temp-1, xTbY-1, yTbY+nTbS_temp+MinCbSizeY*i)) {
			if (pps->constrained_intra_pred_flag == 0 ||
				CurrPic->cu_predmode[(yTbY + nTbS + i*MinCbSizeY) >> MinCbLog2SizeY][(xTbY-1) >> MinCbLog2SizeY] == MODE_INTRA) {
				left_avail[i + 1 + size_in_mincbs] = 1;
				all_unavail = 0;
				//有没有可能图像宽高不能被4整除，最后剩余一个transform block size为2，比如说宽高是奇数 -- 假定不可能
				//如果图像宽度能被4整除，不能被8整除，最后剩余一个Cb 宽为4，4x8的Cb，blkidx不可能到3，只有亮度分量，无色度分量 -- 假定不可能，能被8整除
				//综上，单次循环不可能出现一半在图像内，一半在图像外的情况
				for (j = 0; j < unit_size; j++)
					p_left[nTbS + i*unit_size + j] = src[yTbCmp + nTbS + i*unit_size + j][xTbCmp - 1];
			}
		}
	}
	//up
	for (i = 0; i < size_in_mincbs; i++) {
		if (yTbY > 0) {
			if (pps->constrained_intra_pred_flag == 0 ||
				CurrPic->cu_predmode[(yTbY - 1) >> MinCbLog2SizeY][(xTbY + i*MinCbSizeY) >> MinCbLog2SizeY] == MODE_INTRA) {
				up_avail[i] = 1;
				all_unavail = 0;
				for (j = 0; j < unit_size; j++)
					p_top[i*unit_size + j] = src[yTbCmp - 1][xTbCmp + i*unit_size+j];//fix,1个int类型，1个unsigned char，不能memcpy
			}
		}
	}
	// up right
	for (i = 0; i < size_in_mincbs; i++) {
		if (test_block_available(xTbY + nTbS_temp - 1, yTbY, xTbY + nTbS_temp + MinCbSizeY*i, yTbY - 1)) {
			if (pps->constrained_intra_pred_flag == 0 ||
				CurrPic->cu_predmode[(yTbY - 1) >> MinCbLog2SizeY][(xTbY + nTbS + i*MinCbSizeY) >> MinCbLog2SizeY] == MODE_INTRA) {
				up_avail[i+size_in_mincbs] = 1;
				all_unavail = 0;
				for (j = 0; j < unit_size; j++)
					p_top[nTbS + i*unit_size + j] = src[yTbCmp - 1][xTbCmp + nTbS + i*unit_size + j];
			}
		}
	}

	//8.4.4.2.2 Reference sample substitution process for intra sample prediction
	if (all_unavail) {
		for (i = 0; i < 258; i++) {
			line_buf[i] = 128; // 1  <<  ( bitDepth − 1 )
		}
	} else {
		//左下,最下
		if (left_avail[size_in_mincbs * 2] == 0) {
			found = 0;
			for (i = 2*size_in_mincbs-1; i >= 0; i--) {
				if (left_avail[i]) {
					found = 1;
					for (j = 0; j < unit_size; j++)
						p_left[2*nTbS - unit_size + j] = p_left[unit_size*i - 1];
					left_avail[size_in_mincbs * 2] = 1;
					break;
				}
			}
			if (found == 0) {
				for (i = 0; i < size_in_mincbs * 2; i++) {
					if (up_avail[i]) {
						for (j = 0; j < unit_size; j++)
							p_left[2 * nTbS - unit_size + j] = p_top[unit_size*i];
						left_avail[size_in_mincbs * 2] = 1;
						break;
					}
				}
			}
		}

		//左边+左下,从倒数第二个左下开始
		for (i = 2*size_in_mincbs - 1; i >= 0; i--) {
			if (left_avail[i] == 0) {
				if (i == 0) {
					p_left[-1] = p_left[0];
					p_top[-1] = p_left[-1];
				 } else
					for (j = 0; j < unit_size; j++)
						p_left[(i-1)*unit_size+j] = p_left[unit_size*i];
				left_avail[i] = 1;
			}
		}
		//上+右上
		for (i = 0; i < size_in_mincbs * 2; i++) {
			if (up_avail[i] == 0) {
				for (j = 0; j < unit_size; j++)
					p_top[i*unit_size+j] = p_top[unit_size*i-1];
				up_avail[i] = 1;
			}
		}
	}
	Log_data("left: ");
	for (i = -1; i < 2 * nTbS; i++)
		Log_data("%d ", p_left[i]);
	Log_data("\n");

	Log_data("top: ");
	for (i = -1; i < 2 * nTbS; i++)
		Log_data("%d ", p_top[i]);
	Log_data("\n");

 // 4  |  5 |  6 |  7 |  8  |
 //------------------------
 // 3  |    |    |
// ----|---------|
//  2  |    |    |
// ----|---------|
//  1  |  
// ----|
//  0  |
//     |
//8.4.4.2.2  Reference sample substitution process for intra sample prediction
//8x8的块，对于0(left bottom最下的一块)，依次搜0，1，2，3，...，到第一个可用，用第一个可用代替
//然后依次1,2,3...，1不可用，用0代替，2不可用，用1代替, 5不可用，用4代替
	//intra_smoothing_disabled_flag在sps_range_extension
	if (cIdx == 0 && nTbS != 4 && predModeIntra != INTRA_DC) {
		int minDistVerHor = min(abs(predModeIntra - 26), abs(predModeIntra - 10));
		int intraHorVerDisThres[] = {7, 1, 0};
		if (minDistVerHor > intraHorVerDisThres[log2TrafoSize - 3]) {
			int *pF_left = line_buf + 259;
			int *pF_top = line_buf + 388;
			//biIntFlag=1
			//8.4.4.2.3  Filtering process of neighbouring samples
			if (sps->strong_intra_smoothing_enabled_flag == 1 &&
				nTbS == 32 &&//只有32x32的TU才strong smooth filter
				abs(p_top[-1] + p_top[63] - 2 * p_top[31]) < 8 &&
				abs(p_left[-1] + p_left[63] - 2 * p_left[31]) < 8) { //1<<(BitDepthY-5)
				pF_top[-1] = p_top[-1];
				pF_left[-1] = p_left[-1];
				for (i = 0; i < 63; i++)
					pF_left[i] = ((63 - i) * p_left[-1] + (i + 1) * p_left[63] + 32) >> 6;
				pF_left[63] = p_left[63];
				for (i = 0; i < 63; i++)
					pF_top[i] = ((63 - i) * p_top[-1] + (i + 1) * p_top[63] + 32) >> 6;	
				pF_top[63] = p_top[63];
			} else {
				pF_top[-1] = pF_left[-1] = (pF_top[-1] = p_left[0] + 2 * p_left[-1] + p_top[0] + 2) >> 2;
				for (i = 0; i <= 2 * nTbS - 2; i++)
					pF_left[i] = (p_left[i+1] + 2 * p_left[i] + p_left[i-1] + 2) >> 2;
				pF_left[2 * nTbS - 1] = p_left[2 * nTbS - 1];
				for (i = 0; i <= 2 * nTbS - 2; i++)
					pF_top[i] = (p_top[i+1] + 2 * p_top[i] + p_top[i-1] + 2) >> 2;
				pF_top[2*nTbS-1] = p_top[2*nTbS-1];
			}
			p_left = pF_left;
			p_top = pF_top;
			Log_data("filtered left: ");
			for (i = -1; i < 2 * nTbS; i++)
				Log_data("%d ", p_left[i]);
			Log_data("\n");

			Log_data("filtered top: ");
			for (i = -1; i < 2 * nTbS; i++)
				Log_data("%d ", p_top[i]);
			Log_data("\n");
		}
	}

	int x, y;
	if (predModeIntra == INTRA_PLANAR) {
		Log_data("PLANAR\n");
		for (y = 0; y < nTbS; y++) {
			for (x = 0; x < nTbS; x++) {
				predSamples[y][x] = ((nTbS- 1 - x) * p_left[y] +
					(x + 1) * p_top[nTbS] +
					(nTbS - 1 - y) * p_top[x] +
					(y + 1) * p_left[nTbS] + nTbS) >> (log2TrafoSize + 1);
				Log_data("%d ", predSamples[y][x]);
			}
			Log_data("\n");
		}
	}
	else if (predModeIntra == INTRA_DC) {

		short dcVal = nTbS;
		for (i = 0; i < nTbS; i++)
			dcVal += (p_left[i] + p_top[i]);
		dcVal = dcVal >> (log2TrafoSize + 1);
		Log_data("dcVal %d\n", dcVal);
		for (y = 0; y < nTbS; y++) {
			for (x = 0; x < nTbS; x++) {
				predSamples[y][x] = dcVal;
			}
		}
		if (cIdx == 0 && nTbS < 32) {
			predSamples[0][0] = (p_left[0] + 2 * dcVal + p_top[0] + 2) >> 2;
			Log_data("POS(0,0) %d\n", predSamples[0][0]);
			Log_data("POS(x,0) ");
			for (x = 1; x < nTbS; x++) {
				predSamples[0][x] = (p_top[x] + 3 * dcVal + 2) >> 2;
				Log_data("%d ", predSamples[0][x]);
			}
			Log_data("\n");
			Log_data("POS(0,y) ");
			for (y = 1; y < nTbS; y++) {
				predSamples[y][0] = (p_left[y] + 3 * dcVal + 2) >> 2;
				Log_data("%d ", predSamples[y][0]);
			}
			Log_data("\n");
		}
	} else {
		static int ref_array[130];
		int *ref;
		static int intraPredAngleTab[] = {
			32,  26,  21,  17, 13,  9,  5, 2, 0, -2, -5, -9, -13, -17, -21, -26, -32,
			-26, -21, -17, -13, -9, -5, -2, 0, 2,  5,  9, 13,  17,  21,  26,  32
		};
		static const int invAngleTab[] = {
			-4096, -1638, -910, -630, -482, -390, -315, -256, -315, -390, -482,
			-630, -910, -1638, -4096
		};
		int intraPredAngle = intraPredAngleTab[predModeIntra-2];
		int invAngle = invAngleTab[predModeIntra - 11];
		int iIdx, iFact;
		Log_data("angular mode %d\n", predModeIntra);

		if (predModeIntra >= 18) {
			if (intraPredAngle < 0 &&
				((nTbS * intraPredAngle) >> 5) < -1) {
				ref = ref_array + 64;
				memcpy(ref, p_top - 1, (nTbS + 1) * sizeof(int));
				for (x = -1; x > (nTbS * intraPredAngle) >> 5; x--)
					ref[x] = p_left[-1+((x*invAngle+128)>>8)];
			} else {
				ref = p_top - 1;
			}
			for (y = 0; y < nTbS; y++) {
				iIdx = ((y + 1) * intraPredAngle) >> 5;
				iFact = ((y + 1) * intraPredAngle) & 31;
				Log_data("fact %d data ", iFact);
				if (iFact != 0) {
					for (x = 0; x < nTbS; x++) {
						predSamples[y][x] =
							((32 - iFact) * ref[x + iIdx + 1] + iFact * ref[x + iIdx + 2] + 16) >> 5;
						Log_data("%d ", predSamples[y][x]);
					}
				} else {
					memcpy(&predSamples[y][0], &ref[iIdx + 1], nTbS * sizeof(int));
					for (x = 0; x < nTbS; x++) {
						Log_data("%d ", predSamples[y][x]);
					}
				}
				Log_data("\n");
			}
			if (predModeIntra == 26 && cIdx == 0 && nTbS < 32) {
				Log_data("POS(0,y) ");
				for (y = 0; y < nTbS; y++) { //x = 0, y = 0..nTbS − 1
					predSamples[y][0] = Clip3(0, 255, p_top[0] + ((p_left[y] - p_left[-1]) >> 1));
					Log_data("%d ", predSamples[y][0]);
				}
				Log_data("\n");
			}
		} else {
			if (intraPredAngle < 0 &&
				((nTbS * intraPredAngle) >> 5) < -1) {
				ref = ref_array + 64;
				memcpy(ref, p_left - 1, (nTbS + 1) * sizeof(int));
				for (x = -1; x > (nTbS * intraPredAngle) >> 5; x--)
					ref[x] = p_top[-1+((x*invAngle+128)>>8)];
			} else {
				ref = p_left - 1;
			}
			for (x = 0; x < nTbS; x++) {
				iIdx = ((x + 1) * intraPredAngle) >> 5;
				iFact = ((x + 1) * intraPredAngle) & 31;
				Log_data("fact %d data ", iFact);
				if (iFact != 0) {
					for (y = 0; y < nTbS; y++) {
						predSamples[y][x] =
							((32 - iFact) * ref[y + iIdx + 1] + iFact * ref[y + iIdx + 2] + 16) >> 5;
						Log_data("%d ", predSamples[y][x]);
					}
				} else {
					for (y = 0; y < nTbS; y++) {
						predSamples[y][x] = ref[y + iIdx + 1];
						Log_data("%d ", predSamples[y][x]);
					}
				}
				Log_data("\n");
			}
			if (predModeIntra == 10 && cIdx == 0 && nTbS < 32) {
				Log_data("POS(x,0) ");
				for (x = 0; x < nTbS; x++) { //x = 0..nTbS  − 1, y = 0
					predSamples[0][x] = Clip3(0, 255, p_left[0] + ((p_top[x] - p_top[-1]) >> 1));
					Log_data("%d ", predSamples[0][x]);
				}
				Log_data("\n");
			}
		}
	}

}



//xC, yC CU相对图像坐标
//xB, yB PU在CU的偏移
void inter_pred(int xC, int yC, int xB, int yB, int nPbW, int nPbH, int mv[], int refIdx)
{
	int xIntL, yIntL, xFracL, yFracL;
	int xIntC, yIntC, xFracC, yFracC;
	int xL, yL, xc, yc;
	int mvC[2];
	unsigned char **refPicL, **refPicCb, **refPicCr;
	int xP = xC + xB, yP = yC + yB;
	int i, j;
	int xAIJ, yAIJ, xBIJ, yBIJ;
	int shift1 = 0; //BitDepthY-8
	int shift2 = 6;
	int shift3 = 6; //14-BitDepthY
	int pred_intermediate[8];
	int _shift1 = 6; //14-bitDepth
	int _shift2 = 7; //15-bitDepth
	int _offset1 = 1<<5; //If shift1 is greater than 0, offset1 is set equal to 1 << (shift1 − 1).Otherwise (shift1 is equal to 0), offset1 is set equal to 0.
	int _offset2 = 1<<6; //1 << (shift2 − 1)
	struct HEVCFrame *ref = CurrPic->rpl->ref[refIdx];
	refPicL = ref->SL;
	refPicCb = ref->SCb;
	refPicCr = ref->SCr;
	static int qpel_filters[3][8] = { 
		{-1,  4,-10, 58, 17, -5,  1,  0, },
		{-1,  4,-11, 40, 40,-11,  4, -1, },
		{ 0,  1, -5, 17, 58,-10,  4, -1, }
	};
	static int epel_filters[7][4] = {
		{ -2, 58, 10, -2},
		{ -4, 54, 16, -2},
		{ -6, 46, 28, -4},
		{ -4, 36, 36, -4},
		{ -4, 28, 46, -6},
		{ -2, 16, 54, -4},
		{ -2, 10, 58, -2},
	};
	xFracL = mv[0] & 3; // -3 = 0xfffffffd & 3 = 1    d 1101   -2 0xfffffffe 1110  -1 0xffffffff 1111 -7 0xfffffff9
	yFracL = mv[1] & 3;
	//-3 >>2=-1 -4>>2=-2 -2>>2=-1 -1>>2=-1 -7>>3=-1 -15>>3=-2
	Log_data("luma_mc_uni x0 %d y0 %d width %d height %d mv %d %d ref %d\n",
		xP, yP, nPbW, nPbH, mv[0], mv[1], refIdx);
	if (xFracL == 0 && yFracL == 0) {
		Log_data("pel full sample\n");
	}
	else if (yFracL == 0) {
		Log_data("qpel h\n");
	}
	else if (xFracL == 0) {
		Log_data("qpel v\n");
	}
	else {
		Log_data("qpel hv\n");
	}


	for (yL = 0; yL < nPbH; yL++) {
		for (xL = 0; xL < nPbW; xL++) {
			xIntL = xP + (mv[0] >> 2) + xL;
			yIntL = yP + (mv[1] >> 2) + yL;
			if (xP == 144 && yP == 8 && slice_num == 2 && yL == 1 && xL == 6)
				xP = xP;
			predSamplesL[yB + yL][xB + xL] = 0;
			if (xFracL == 0 && yFracL == 0) { //整像素
				xAIJ = Clip3(0, PicWidthInSamplesY - 1, xIntL);
				yAIJ = Clip3(0, PicHeightInSamplesY - 1, yIntL);
				predSamplesL[yB+yL][xB+xL] = refPicL[yAIJ][xAIJ] << shift3;
			} else if (yFracL == 0) { //a00,b00,c00, 插值用到横向7/8点
				yAIJ = Clip3(0, PicHeightInSamplesY - 1, yIntL);
				for (i = -3; i <= 4; i++) {
					xAIJ = Clip3(0, PicWidthInSamplesY-1, xIntL + i);
					predSamplesL[yB+yL][xB+xL] += qpel_filters[xFracL-1][i+3] * refPicL[yAIJ][xAIJ];
				}
			} else if (xFracL == 0) { //d00,h00,n00, 插值用到纵向7/8点
				xAIJ = Clip3(0, PicWidthInSamplesY - 1, xIntL);
				for (i = -3; i <= 4; i++) {
					yAIJ = Clip3(0, PicHeightInSamplesY-1, yIntL + i);
					predSamplesL[yB+yL][xB+xL] += qpel_filters[yFracL-1][i+3] * refPicL[yAIJ][xAIJ];
				}
			} else { //插值用到纵向的7/8个中间点，每个中间点的y轴为整像素，计算中间点用到横向7/8个点
				memset(pred_intermediate, 0, 32);
				for (i = -3; i <= 4; i++) {
					yAIJ = Clip3(0, PicHeightInSamplesY-1, yIntL + i);
					for (j = -3; j <= 4; j++) {
						xAIJ = Clip3(0, PicWidthInSamplesY-1, xIntL + j);
						pred_intermediate[i+3] += qpel_filters[xFracL-1][j+3] * refPicL[yAIJ][xAIJ];
					}
				}
				
				for (i = -3; i <= 4; i++) {
					predSamplesL[yB+yL][xB+xL] += qpel_filters[yFracL-1][i+3] * pred_intermediate[i+3];
				}
				predSamplesL[yB+yL][xB+xL] >>= shift2;

			}

			//fix 8.5.3.3.4	Weighted sample prediction process
			predSamplesL[yB + yL][xB + xL] = Clip3(0, 255, (predSamplesL[yB+yL][xB+xL] + _offset1) >> _shift1);
			Log_data("%d ", predSamplesL[yB + yL][xB + xL]);
		}
		Log_data("\n");
	}
	mvC[0] = mv[0];
	mvC[1] = mv[1];
	xFracC = mv[0] & 7;
	yFracC = mv[1] & 7;

#if 0
	for (yc = 0; yc < nPbH/2; yc++) {
		for (xc = 0; xc < nPbW/2; xc++) {
			xIntC = xP/2 + (mv[0] >> 3) + xc;
			yIntC = yP/2 + (mv[1] >> 3) + yc;
			xFracC = mv[0] & 7;
			yFracC = mv[1] & 7;
			predSamplesCb[yB/2+yc][xB/2+xc] = 0;
			predSamplesCr[yB/2+yc][xB/2+xc] = 0;
			if (xFracC == 0 && yFracC == 0) { //整像素
				xBIJ = Clip3(0, PicWidthInSamplesY/2-1, xIntC);
				yBIJ = Clip3(0, PicHeightInSamplesY/2-1, yIntC);
				predSamplesCb[yB/2+yc][xB/2+xc] = refPicCb[yBIJ][xBIJ] << shift3;
				predSamplesCr[yB/2+yc][xB/2+xc] = refPicCr[yBIJ][xBIJ] << shift3;
			} else if (yFracC == 0) {
				yBIJ = Clip3(0, PicHeightInSamplesY/2-1, yIntC);
				for (i = -1; i <= 2; i++) {
					xBIJ = Clip3(0, PicWidthInSamplesY/2-1, xIntC + i);
					predSamplesCb[yB/2+yc][xB/2+xc] += epel_filters[xFracC-1][i+1] * refPicCb[yBIJ][xBIJ];
					predSamplesCr[yB/2+yc][xB/2+xc] += epel_filters[xFracC-1][i+1] * refPicCr[yBIJ][xBIJ];
				}
			} else if (xFracC == 0) {
				xBIJ = Clip3(0, PicWidthInSamplesY/2-1, xIntC);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY/2-1, yIntC + i);
					predSamplesCb[yB/2+yc][xB/2+xc] += epel_filters[yFracC-1][i+1] * refPicCb[yBIJ][xBIJ];
					predSamplesCr[yB/2+yc][xB/2+xc] += epel_filters[yFracC-1][i+1] * refPicCr[yBIJ][xBIJ];
				}
			} else {
				memset(pred_intermediate, 0, 32);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY/2-1, yIntC + i);
					for (j = -1; j <= 2; j++) {
						xBIJ = Clip3(0, PicWidthInSamplesY/2-1, xIntC + j);
						pred_intermediate[i+1] += epel_filters[xFracC-1][j+1] * refPicCb[yBIJ][xBIJ];
						pred_intermediate[i+5] += epel_filters[xFracC-1][j+1] * refPicCr[yBIJ][xBIJ];
					}
				}
				
				for (i = -1; i <= 2; i++) {
					predSamplesCb[yB/2+yc][xB/2+xc] += epel_filters[yFracC-1][i+1] * pred_intermediate[i+1];
					predSamplesCr[yB/2+yc][xB/2+xc] += epel_filters[yFracC-1][i+1] * pred_intermediate[i+5];
				}
				predSamplesCb[yB/2+yc][xB/2+xc] >>= shift2;
				predSamplesCr[yB/2+yc][xB/2+xc] >>= shift2;
				predSamplesCb[yB / 2 + yc][xB / 2 + xc] = Clip3(0, 255, (predSamplesCb[yB / 2 + yc][xB / 2 + xc] + _offset1) >> _shift1);
				predSamplesCr[yB / 2 + yc][xB / 2 + xc] = Clip3(0, 255, (predSamplesCr[yB / 2 + yc][xB / 2 + xc] + _offset1) >> _shift1);
			}
		}
	}
#endif

#if 1
	if (xFracC == 0 && yFracC == 0) {
		Log_data("pel full sample\n");
	}
	else if (yFracC == 0) {
		Log_data("epel h\n");
	}
	else if (xFracC == 0) {
		Log_data("epel v\n");
	}
	else {
		Log_data("epel hv\n");
	}
	for (yc = 0; yc < nPbH / 2; yc++) {
		for (xc = 0; xc < nPbW / 2; xc++) {
			xIntC = xP / 2 + (mv[0] >> 3) + xc;
			yIntC = yP / 2 + (mv[1] >> 3) + yc;
			predSamplesCb[yB / 2 + yc][xB / 2 + xc] = 0;
			if (xFracC == 0 && yFracC == 0) { //整像素
				xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC);
				yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC);
				predSamplesCb[yB / 2 + yc][xB / 2 + xc] = refPicCb[yBIJ][xBIJ] << shift3;

			}
			else if (yFracC == 0) {
				yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC);
				for (i = -1; i <= 2; i++) {
					xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC + i);
					predSamplesCb[yB / 2 + yc][xB / 2 + xc] += epel_filters[xFracC-1][i + 1] * refPicCb[yBIJ][xBIJ];

				}

			}
			else if (xFracC == 0) {
				xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC + i);
					predSamplesCb[yB / 2 + yc][xB / 2 + xc] += epel_filters[yFracC-1][i + 1] * refPicCb[yBIJ][xBIJ];

				}

			}
			else {
				memset(pred_intermediate, 0, 32);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC + i);
					for (j = -1; j <= 2; j++) {
						xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC + j);
						pred_intermediate[i + 1] += epel_filters[xFracC-1][j + 1] * refPicCb[yBIJ][xBIJ];
					}
				}

				for (i = -1; i <= 2; i++) {
					predSamplesCb[yB / 2 + yc][xB / 2 + xc] += epel_filters[yFracC-1][i + 1] * pred_intermediate[i + 1];
				}
				predSamplesCb[yB / 2 + yc][xB / 2 + xc] >>= shift2;

			}
			predSamplesCb[yB / 2 + yc][xB / 2 + xc] = Clip3(0, 255, (predSamplesCb[yB / 2 + yc][xB / 2 + xc] + _offset1) >> _shift1);
			Log_data("%d ", predSamplesCb[yB / 2 + yc][xB / 2 + xc]);
		}
		Log_data("\n");
	}
	if (xFracC == 0 && yFracC == 0) {
		Log_data("pel full sample\n");
	}
	else if (yFracC == 0) {
		Log_data("epel h\n");
	}
	else if (xFracC == 0) {
		Log_data("epel v\n");
	}
	else {
		Log_data("epel hv\n");
	}
	for (yc = 0; yc < nPbH / 2; yc++) {
		for (xc = 0; xc < nPbW / 2; xc++) {
			xIntC = xP / 2 + (mv[0] >> 3) + xc;
			yIntC = yP / 2 + (mv[1] >> 3) + yc;
			xFracC = mv[0] & 7;
			yFracC = mv[1] & 7;
			predSamplesCr[yB / 2 + yc][xB / 2 + xc] = 0;
			if (xFracC == 0 && yFracC == 0) { //整像素
				xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC);
				yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC);
				predSamplesCr[yB / 2 + yc][xB / 2 + xc] = refPicCr[yBIJ][xBIJ] << shift3;

			}
			else if (yFracC == 0) {
				yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC);
				for (i = -1; i <= 2; i++) {
					xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC + i);
					predSamplesCr[yB / 2 + yc][xB / 2 + xc] += epel_filters[xFracC-1][i + 1] * refPicCr[yBIJ][xBIJ];
				}

			}
			else if (xFracC == 0) {
				xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC + i);
					predSamplesCr[yB / 2 + yc][xB / 2 + xc] += epel_filters[yFracC-1][i + 1] * refPicCr[yBIJ][xBIJ];
				}

			}
			else {
				memset(pred_intermediate, 0, 32);
				for (i = -1; i <= 2; i++) {
					yBIJ = Clip3(0, PicHeightInSamplesY / 2 - 1, yIntC + i);
					for (j = -1; j <= 2; j++) {
						xBIJ = Clip3(0, PicWidthInSamplesY / 2 - 1, xIntC + j);
						pred_intermediate[i + 5] += epel_filters[xFracC-1][j + 1] * refPicCr[yBIJ][xBIJ];
					}
				}

				for (i = -1; i <= 2; i++) {
					predSamplesCr[yB / 2 + yc][xB / 2 + xc] += epel_filters[yFracC-1][i + 1] * pred_intermediate[i + 5];
				}
				predSamplesCr[yB / 2 + yc][xB / 2 + xc] >>= shift2;

			}
			predSamplesCr[yB / 2 + yc][xB / 2 + xc] = Clip3(0, 255, (predSamplesCr[yB / 2 + yc][xB / 2 + xc] + _offset1) >> _shift1);
			Log_data("%d ", predSamplesCr[yB / 2 + yc][xB / 2 + xc]);
		}
		Log_data("\n");
	}
#endif

}
