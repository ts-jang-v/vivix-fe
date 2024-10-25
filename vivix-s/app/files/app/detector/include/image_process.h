/******************************************************************************/
/**
 * @file    image_process.h
 * @brief   
 * @author  한명희
*******************************************************************************/
#ifndef _VW_IMAGE_PROCESS_H_
#define _VW_IMAGE_PROCESS_H_

/*******************************************************************************
*	Include Files
*******************************************************************************/
#include "typedef.h"

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
typedef struct _defect_pixel_info
{
	u8 	b_defect_type;
	u8	b_dummy;
	u16	w_available_line;
	u32	n_src_info;
	u16 n_position_x;
	u16 n_position_y;
}__attribute__((packed)) defect_pixel_info_t;


/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/


/*******************************************************************************
*	Variable Definitions
*******************************************************************************/


/*******************************************************************************
*	Function Prototypes
*******************************************************************************/

class CImageProcess
{
private:
    
    int		(* print_log)(int level,int print,const char * format, ...);
    s8		m_p_log_id[15];
    
    u32		m_n_width;
    u32		m_n_height;
    
    u32		m_n_quarter_width;
    u32		m_n_quarter_height;
    
    bool	m_b_gain;
    float*	m_p_gain;
        
    bool	m_b_defect;
    defect_pixel_info_t*	m_p_defect_info_t;
    u32						m_n_defect_total_count;
    u32						m_n_idx; //invoked defect count
    
    void	defect_correction_vline( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info );
    void	defect_correction_hline( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info );
    void	defect_correction_pixel( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info );
    
    int		GetProjectionData(u16* ptrImage, u16* ptrProjectedX, u16* ptrProjectedY, int xDim, int yDim);
    int		GetY1ColliPosition2(u16* ptrProjectionY, int yStart, int yEnd, int dim, int threshold);
    int		GetY2ColliPosition2(u16* ptrProjectionY, int yStart, int yEnd, int dim, int threshold);
    bool	AvgHistogram(u16* ptrHistogram, int sizeofHistogram, int sizeofKernel);
    bool	GetPeakHistogram(u16* ptrHist, int dimension, int sizeofKernel);
    int		GetX1ColliPosition2(u16* ptrProjectionX, int xStart, int xEnd, int dim, int threshold);
    int		GetX2ColliPosition2(u16* ptrProjectionX, int xStart, int xEnd, int dim, int threshold);
    int		ShrinkImage(u16* ptrSource, u16* ptrTarget, int xDim, int yDim, int shrinkFactor);
    int		GetHistogram(u16* ptrSource, u16* ptrHistogram, int histogramSize, int xDim, int yDim);
    int		AverageFilter(u16* ptrSource, u16* ptrTarget, int xDim, int yDim, int kernelSize);
    bool	GetAutoWindow(u16* ptrImage, int xDim, int yDim, int flatGamma, int postGamma, int maxValue, int blackOffset, int whiteOffset, int* window, int* level, int xStart, int xEnd, int yStart, int yEnd, int resizeFactor);
    bool	AutoROI(u16* _pImage, int _nWidth, int _nHeight, int _nUsingBits, int * _nWindowValue, int * _nLevelValue, int _nCompGamma, int _nPostGamma, int _nMaxValue);
    void	Mapping(u16 * pImageBuf, int nWidth, int nHeight, long srcLow, long srcHigh, long desLow, long desHigh);
    void	apply_gain(u16* image);
    bool	window_level_lut_image(unsigned short* pIn, unsigned short* pOut, unsigned short *pLut, unsigned int nInUsingBits, unsigned int nOutUsingBits, unsigned int nLutUsingBits, unsigned int nInBytesPerLine, unsigned int nOutBytesPerLine, unsigned int nWidth, unsigned int nHeight);
    
protected:
public:
    
	CImageProcess(int (*log)(int,int,const char *,...), u32 i_n_width, u32 i_n_height );
	CImageProcess(void);
	~CImageProcess();
	
	void	defect_correction( u16* i_p_buffer, u32 i_n_cur_line_index);
	void 	defect_correction_idx_reset(void)	{ m_n_idx = 0; }
	bool	load_lut( const char* i_p_file_name );
	bool	load_gain( const char* i_p_file_name );
	//bool	defect_load( const char* i_p_file_name );
	bool	defect_load( u32 i_n_total_count, FILE* i_p_file );
	void	defect_print(void);
	
	bool	SaveMono14ToBMP(const s8* i_p_file_name, u8* pImage );
	void	process( u16* i_p_image, bool i_b_invert );
	
	bool	defect_get_enable(void){return m_b_defect;}
	void	defect_set_enable( bool i_b_on ){ m_b_defect = i_b_on; }

	void	gain_on( bool i_b_on=true ){ m_b_gain = i_b_on; }
    
};


#endif /* end of _VW_IMAGE_PROCESS_H_*/

