
写verilog实现H265解码器前为了理解协议实现的一个简化版本的H265解码器C代码，是VS2013的工程，输入文件是test.265,工程里含有一些测试文件，重命令成test.265即可。编译，按F5跑即可，输出out.yuv,以及一些log文件。

主要参考H265标准，ffmpeg和HM，参考的是JCTVC-L1003_v34，
代码尽量和标准写的保持一致，变量名字和标准也保持一致，便于理解，消除看HM和ffmpeg时因为绕弯而带来的困惑，适合初学h265标准的同学们。
举一个cabac decode bin的例子，
HM12.0的代码如下，
Void
TDecBinCABAC::decodeBin( UInt& ruiBin, ContextModel &rcCtxModel )
{
  UInt uiLPS = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) - 4 ];
  m_uiRange -= uiLPS;
  UInt scaledRange = m_uiRange << 7;
  
  if( m_uiValue < scaledRange )
  {
    // MPS path
    ruiBin = rcCtxModel.getMps();
    rcCtxModel.updateMPS();
    
    if ( scaledRange >= ( 256 << 7 ) )
    {
      return;
    }
    
    m_uiRange = scaledRange >> 6;
    m_uiValue += m_uiValue;
    
    if ( ++m_bitsNeeded == 0 )
    {
      m_bitsNeeded = -8;
      m_uiValue += m_pcTComBitstream->readByte();      
    }
  }
  else
  {
    // LPS path
    Int numBits = TComCABACTables::sm_aucRenormTable[ uiLPS >> 3 ];
    m_uiValue   = ( m_uiValue - scaledRange ) << numBits;
    m_uiRange   = uiLPS << numBits;
    ruiBin      = 1 - rcCtxModel.getMps();
    rcCtxModel.updateLPS();
    
    m_bitsNeeded += numBits;
    
    if ( m_bitsNeeded >= 0 )
    {
      m_uiValue += m_pcTComBitstream->readByte() << m_bitsNeeded;
      m_bitsNeeded -= 8;
    }
  }
}

FFMPEG的代码，
static av_always_inline int get_cabac_inline(CABACContext *c, uint8_t * const state){
    int s = *state;
    int RangeLPS= ff_h264_lps_range[2*(c->range&0xC0) + s];
    int bit, lps_mask;


    c->range -= RangeLPS;
    lps_mask= ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

    s^=lps_mask;
    *state= (ff_h264_mlps_state+128)[s];
    bit= s&1;


    lps_mask= ff_h264_norm_shift[c->range];
    c->range<<= lps_mask;
    c->low  <<= lps_mask;

    if(!(c->low & CABAC_MASK)) {
        refill2(c);

    }
    return bit;
}
下面是完完全全按照spec两幅图来实现的代码，实践证明代码完全按照spec来写，结果也是对的，而HM和ffpeg都有不同程度的绕弯，让人在理解过程中感到蛋腾。





void decode_bin(unsigned char *rbsp, int *binVal, struct context_model *cm)
{
	//9.3.4.3.2  Arithmetic decoding process for a binary decision
	int ivlLpsRange, qRangeIdx;
	qRangeIdx = (ivlCurrRange >> 6) & 3;
	ivlLpsRange = rangeTabLps[cm->pStateIdx][qRangeIdx];

	ivlCurrRange = ivlCurrRange - ivlLpsRange;
	if (ivlOffset >= ivlCurrRange) {
		//LPS path
		*binVal = 1 - cm->valMps;
		ivlOffset -= ivlCurrRange;
		ivlCurrRange = ivlLpsRange;
	} else {
		//MPS path
		*binVal = cm->valMps;
	}
	//9.3.4.3.2.2  State transition process 
	if(*binVal == cm->valMps){ 
		cm->pStateIdx = transIdxMps[cm->pStateIdx];
	} else {
		if (cm->pStateIdx == 0) 
			cm->valMps = 1 - cm->valMps;

		cm->pStateIdx = transIdxLps[cm->pStateIdx];
	}
	//renorm
	while (ivlCurrRange < 256) {
		ivlCurrRange = ivlCurrRange << 1;
		ivlOffset = ivlOffset << 1;
		ivlOffset = ivlOffset | read_one_bit(rbsp);
	}

}


