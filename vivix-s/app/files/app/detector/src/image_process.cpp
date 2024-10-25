/*******************************************************************************
 모  듈 : image_process
 작성자 : 한명희
 버  전 : 0.0.1
 설  명 : 영상 처리 모듈
 참  고 : 

 버  전:
         0.0.1  최초 작성
*******************************************************************************/



/*******************************************************************************
*	Include Files
*******************************************************************************/
#include <iostream>
#include <stdio.h>			// fprintf()
#include <string.h>			// memset() memset/memcpy
#include <stdlib.h>			// malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <stdbool.h>		// bool, true, false
#include <assert.h>
#include <errno.h>            // errno
#include <sys/poll.h>		// struct pollfd, poll()

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>		// mkfifo
#include <fcntl.h>			// open
#include <math.h>			// pow

#include "image_process.h"

using namespace std;

/*******************************************************************************
*	Constant Definitions
*******************************************************************************/
#define swap_16(A) ((((u16)(A) & 0xff00) >> 8) | \
					(((u16)(A) & 0x00ff) << 8))

#define swap_32(A) ((((u32)(A) & 0xff000000) >> 24) | \
					 (((u32)(A) & 0x00ff0000) >> 8) | \
					 (((u32)(A) & 0x0000ff00) << 8) | \
					 (((u32)(A) & 0x000000ff) << 24))

#define WORDOFFSET            32768
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#define MAX(a,b)  ((a) < (b) ? (b) : (a))

#define MAXVALUE_FROM_BIT(X) ((1<<X)-1)

/*******************************************************************************
*	Type Definitions
*******************************************************************************/
/* BITMAPFILEHEADER 구조체 */
typedef struct tagBITMAPFILEHEADER{ //bmfh
	u16 bfType;         // 파일의 형태, 0x42, 0x4d (BM) 이어야함
	u32 bfSize;         // 비트맵 파일의 크기 (Byte단위)
	u16 bfReserved1;   // 예약. 0으로 설정   
	u16 bfReserved2;  // 예약2. 0으로설정
	u32 bfOffBits;     // 실제 비트맵데이터까지의 오프셋값 
   // 실제로는 bfOffBits = BITMAPFILEHEADER크기 + BITMAPINFOHEADER크기 + RGBQUAD 구조체배열의크기 이다. (그림을 봐야 이해할수 있을것이다)
} __attribute__((packed))BITMAPFILEHEADER;
 
/* BITMAPINFOHEADER 구조체 */
typedef struct tagBITMAPINFOHEADER{ //bmfh
	u32 biSize;         // 이 구조체의 크기. 구조체 버전확인할수 있다.
	int biWidth;        // 비트맵의 가로 픽셀수
	int biHeight;       // 비트맵의 세로 픽셀수
	u16 biPlanes;      // 플레인의 갯수 반드시 1이어야함 
	u16 biBitCount;     // 한 픽셀이 구성되는 비트의수
	u32 biCompression; // 압축방법. BI_RGB일땐 비압축 BI_RLE8, BI_RLE4인경우 run length encode방법으로 압축
	u32 biSizeImage;   // 이미지의 크기. 압축이 안되어있을때는 0
	int biXPelsPerMeter; // 가로 해상도
	int biYPelsPerMeter; // 세로 해상도
	u32 biClrUsed;  // 색상테이블을 사용하였을때 실제 사용되는 색상수
	u32 biClrImportant; // 비트맵을 출력하는데 필수 색상수
} __attribute__((packed))BITMAPINFOHEADER;
 
/* RGBQUAD구조체 */
typedef struct tagRGBQUAD{ //rgbq
	u8 rgbBlue;       // Blue Value 
	u8 rgbGreen;     // Green Vlaue
	u8 rgbRed;       // Red Value
	u8 rgbReserved; // 실제 사용하지 않음. 0
} __attribute__((packed))RGBQUAD;


typedef struct tagBITMAPINFO{ //bmi
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD   bmiColors[1];
} __attribute__((packed))BITMAPINFO;


typedef enum
{
	eDEFECT_PIXEL = 0,
	eDEFECT_LINE_H = 1,
	eDEFECT_LINE_V = 2,	
	eDEFECT_NONE = 3,
} DEFECT_TYPE;



/*******************************************************************************
*	Macros (Inline Functions) Definitions
*******************************************************************************/

/*******************************************************************************
*	Variable Definitions
*******************************************************************************/
static unsigned short s_p_lut[65536]={0,};

/*******************************************************************************
*	Function Prototypes
*******************************************************************************/



/******************************************************************************/
/**
* \brief        constructor
* \param        log         log function
* \return       none
* \note
*******************************************************************************/
CImageProcess::CImageProcess(int (*log)(int,int,const char *,...), u32 i_n_width, u32 i_n_height )
{
    print_log = log;
    strcpy( m_p_log_id, "IMAGE PROCESS " );	// 14
    
    m_n_width = i_n_width;
    m_n_height = i_n_height;
    
    m_n_quarter_width	= i_n_width/4;
    m_n_quarter_height	= i_n_height/4;
    
    m_n_idx = 0; //invoked defect count
    m_p_gain = NULL;
    
    m_b_defect = false;
    m_p_defect_info_t = NULL;
    m_n_defect_total_count = 0;

    print_log( DEBUG, 1, "[%s] CImageProcess(width: %d, height: %d)\n", m_p_log_id, m_n_quarter_width, m_n_quarter_height );
}
/******************************************************************************/
/**
* \brief        constructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImageProcess::CImageProcess(void)
{
}

/******************************************************************************/
/**
* \brief        destructor
* \param        none
* \return       none
* \note
*******************************************************************************/
CImageProcess::~CImageProcess()
{
	safe_free( m_p_gain );
	safe_free( m_p_defect_info_t );
    
    print_log( DEBUG, 1, "[%s] ~CImageProcess\n", m_p_log_id );
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::load_lut( const char* i_p_file_name )
{
	FILE* fp = NULL;
	int numberOfControlPoint = 0;
	int numberOfBits = 0;
	int idx = 0;
	int nCount = 0;

	fp = fopen( i_p_file_name, "rb");
	if(fp != NULL)
	{
		// read number of control point
		fread(&numberOfControlPoint,1, sizeof(int), fp);
		print_log(INFO,1,"[%s] numberOfControlPoint : %d\n", m_p_log_id, numberOfControlPoint);
		fseek(fp, numberOfControlPoint * 24, SEEK_CUR);
		fread(&numberOfBits,1, sizeof(int), fp);
		print_log(INFO,1,"[%s] numberOfBits : %d\n", m_p_log_id, numberOfBits);
		nCount = 1 << numberOfBits;
		print_log(INFO,1,"[%s] nCount : %d\n", m_p_log_id, nCount);

		for(idx = 0 ; idx < nCount ;  idx ++)
			fread(&s_p_lut[idx], 1, sizeof(unsigned short), fp);

		fclose(fp);
		return true;
	}
	return false;
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetProjectionData(u16* ptrImage, u16* ptrProjectedX, u16* ptrProjectedY, int xDim, int yDim)
{

	 memset(ptrProjectedX, 0, sizeof(u16)*xDim);
	 memset(ptrProjectedY, 0, sizeof(u16)*yDim);

	for (int x=0;x<xDim;x++)
	{
		float buffer = 0.0f;

		for (int y=0;y<yDim;y++)
		{
			buffer += *(ptrImage+y*xDim+x);
		}

			buffer /= (float)yDim;

		if (buffer<0) 
			buffer = 0;
		else if (buffer>65535) 
			buffer = 65535;

		*(ptrProjectedX+x) = (u16)buffer;
	}
	
	for (int y=0;y<yDim;y++)
	{
		float buffer = 0.0f;
		int yBuff = y*xDim;	

		for (int x=0;x<xDim;x++)
		{
			buffer += *(ptrImage+yBuff+x);
		}

		buffer /= (float)xDim;

		if (buffer<0) 
			buffer = 0;
		else if (buffer>65535) 
			buffer = 65535;

		*(ptrProjectedY+y) = (u16)buffer;
	}
	 return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetY1ColliPosition2(u16* ptrProjectionY, int yStart, int yEnd, int dim, int threshold)
{
        int i;
        u16 maxDiff = 0;
        int maxIndex;

        if (yEnd<=yStart) return -1;

        maxIndex = yStart;

        for (i=yStart;i<yEnd;i++)
        {
               if (*(ptrProjectionY+i)>(WORDOFFSET+threshold))
			   {
					   if (*(ptrProjectionY+i)>maxDiff)
                       {
                              maxDiff = *(ptrProjectionY+i);
                              maxIndex = i;
                       }
               }
        }

        return maxIndex;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetY2ColliPosition2(u16* ptrProjectionY, int yStart, int yEnd, int dim, int threshold)
{
        int i;
        u16 maxDiff = 0;
		int maxIndex;

        if (yEnd<=yStart) return -1;

        maxIndex = yEnd;

        for (i=yEnd;i>yStart;i--)
		{
               if (*(ptrProjectionY+i)>(WORDOFFSET+threshold))
			   {
					   if (*(ptrProjectionY+i)>maxDiff)
                       {
                              maxDiff = *(ptrProjectionY+i);
                              maxIndex = i;
                       }
               }
		}

        return maxIndex;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::AvgHistogram(u16* ptrHistogram, int sizeofHistogram, int sizeofKernel)
{
		int i, j;
		int offsetSIze;
		int offset;
		float offsetData;
		u16* dataBuff = NULL;


		dataBuff = (u16 *) malloc(sizeofHistogram * sizeof(u16));

		offsetSIze = sizeofKernel/2;

		for (i=0;i<sizeofHistogram;i++)
        {
               offsetData = 0;
               offset = 0;
			   for (j=-offsetSIze;j<offsetSIze;j++)
			   {
                       if (((i+j)>=0)&&((i+j)<sizeofHistogram))
                       {
                              offsetData += *(ptrHistogram+i+j);
                              offset++;
                       }
               }
			   *(dataBuff+i) = (u16)(offsetData/(float)offset);
        }
        memcpy(ptrHistogram, dataBuff, sizeof(u16)*sizeofHistogram);
        free(dataBuff);
        return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::GetPeakHistogram(u16* ptrHist, int dimension, int sizeofKernel)
{
        int i, j;
        int offsetSIze;
        int offset;
        float offsetData;
        u16* dataBuff;

        dataBuff = (u16*)malloc(sizeof(u16)*dimension);
        if (!dataBuff)
		{
               return false;
        }
        offsetSIze = sizeofKernel/2;

        for (i=0;i<dimension;i++)
        {
			   offsetData = 0;
               offset = 0;
			   for (j=-offsetSIze;j<offsetSIze;j++)
			   {
                       if (((i+j)>=0)&&((i+j)<dimension))
                       {
							  offsetData += *(ptrHist+i+j);
                              offset++;
                       }
               }
               *(dataBuff+i) = *(ptrHist+i);
			   *(dataBuff+i) += WORDOFFSET;
               *(dataBuff+i) -= (u16)(offsetData/(float)offset);
        }
        memcpy(ptrHist, dataBuff, sizeof(u16)*dimension);
        free(dataBuff);
		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetX1ColliPosition2(u16* ptrProjectionX, int xStart, int xEnd, int dim, int threshold)
{
        int i;
        u16 maxDiff = 0;
		int maxIndex;

        if (xEnd<=xStart) return -1;

		maxIndex = xStart;

        for (i=xStart;i<xEnd;i++)
        {
			   if (*(ptrProjectionX+i)>(WORDOFFSET+threshold))
               {
					   if (*(ptrProjectionX+i)>maxDiff)
					   {
							  maxDiff = *(ptrProjectionX+i);
							  maxIndex = i;
					   }
               }
        }

		return maxIndex;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetX2ColliPosition2(u16* ptrProjectionX, int xStart, int xEnd, int dim, int threshold)
{
        int i;
        u16 maxDiff = 0;
        int maxIndex;

		if (xEnd<=xStart) return -1;

        maxIndex = xEnd;

        for (i=xEnd;i>xStart;i--)
		{
               if (*(ptrProjectionX+i)>(WORDOFFSET+threshold))
               {
                       if (*(ptrProjectionX+i)>maxDiff)
					   {
							  maxDiff = *(ptrProjectionX+i);
							  maxIndex = i;
					   }
			   }
		}

		return maxIndex;
}

/******************************************************************************/
/**
 * @brief   영상 축소 함수
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::ShrinkImage(u16* ptrSource, u16* ptrTarget, int xDim, int yDim, int shrinkFactor)
{
	   int yDimTarget;
	   int xDimTarget;
	   float divFactor;

	   xDimTarget = xDim/shrinkFactor;
	   yDimTarget = yDim/shrinkFactor;

	   if ((xDimTarget*shrinkFactor)!=xDim) return -1;                     // Wrong xDim size;
	   if ((yDimTarget*shrinkFactor)!=yDim) return -2;                     // Wrong yDim size;

	   divFactor = (float)(shrinkFactor*shrinkFactor);

	   for (int y=0;y<yDimTarget;y++)
	   {
			   for (int x=0;x<xDimTarget;x++)
			   {
					  float buff = 0.0f;
					  for (int i=0;i<shrinkFactor;i++)
					  {
							  for (int j=0;j<shrinkFactor;j++)
							  {
									 buff += (float)(*(ptrSource + (y*shrinkFactor)*xDim + (x*shrinkFactor) + i*xDim + j));
							  }
					  }
					  buff /= divFactor;
					  //                     buff += 0.5;                                                               // Round 효과
					  if (buff>65535) buff = 65535;
					  else if (buff<0) buff = 0;
					  *(ptrTarget+y*xDimTarget+x) = (u16)buff;
			   }
	   }

		return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::GetHistogram(u16* ptrSource, u16* ptrHistogram, int histogramSize, int xDim, int yDim)
{
		int x, y;

        memset(ptrHistogram, 0, sizeof(u16)*histogramSize);

		for (y=0;y<yDim;y++)
        {
               for (x=0;x<xDim;x++)
               {
					   *(ptrHistogram+*(ptrSource+y*xDim+x)) += 1;
               }
        }
        return 0;
}


/******************************************************************************/
/**
 * @brief   Moving average filter
 * @param   
 * @return  
 * @note    
*******************************************************************************/
int CImageProcess::AverageFilter(u16* ptrSource, u16* ptrTarget, int xDim, int yDim, int kernelSize)
{
	   if (kernelSize<3) return -1;                                                // Kernel size too small
	   if (kernelSize>9) return -2;                                                // Kernel size too big
	   if (xDim<24) return -3;                                                                    // X dimension too small
	   if (xDim>6144) return -4;                                                          // X dimension too big
	   if (yDim<24) return -5;                                                                    // Y dimension too small
	   if (yDim>6144) return -6;                                                          // Y dimension too big

	   int radius = (kernelSize-1)/2;
	   if(radius ==1)
	   {
			   for (int y=1;y<yDim-1;y++)
			   {
					  int yBuff = y*xDim;
					  for (int x=1;x<xDim-1;x++)
					  {
							  int xBuff = yBuff+x;
							  float dataBuff = 0;
							  dataBuff = (float)((float)ptrSource[xBuff-xDim-1]+ptrSource[xBuff-xDim]+ptrSource[xBuff-xDim+1]+ptrSource[xBuff-1]+ptrSource[xBuff]+ptrSource[xBuff+1]+ptrSource[xBuff+xDim-1]+ptrSource[xBuff+xDim]+ptrSource[xBuff+xDim+1]);
							  dataBuff /= 9.0f;
							  // no need to check
							  {
									 //dataBuff = MAX(dataBuff, 0.0f);
									 //dataBuff = MIN(dataBuff, 65535.0f);
							  }
							  *(ptrTarget+xBuff) = (u16)dataBuff;
					  }
			   }

			   // top-left
			   {
					  float dataBuff = 0;
					  dataBuff = (float)((float)ptrSource[0]+ptrSource[1]+ptrSource[xDim]+ptrSource[xDim+1]);
					  dataBuff /= 4.0f;
					  dataBuff = MAX(dataBuff, 0.0f);
					  dataBuff = MIN(dataBuff, 65535.0f);
					  *(ptrTarget) = (u16)dataBuff;
			   }

			   // top-right
			   {
					  float dataBuff = 0;
					  dataBuff = (float)((float)ptrSource[xDim-1]+ptrSource[xDim-1-1]+ptrSource[xDim-1+xDim]+ptrSource[xDim-1+xDim-1]);
					  dataBuff /= 4.0f;
					  dataBuff = MAX(dataBuff, 0.0f);
					  dataBuff = MIN(dataBuff, 65535.0f);
					  *(ptrTarget+xDim-1) = (u16)dataBuff;
			   }

			   // bottom-left
			   {
					  float dataBuff = 0;
					  dataBuff = (float)((float)ptrSource[xDim*(yDim-1)]+ptrSource[xDim*(yDim-1)+1]+ptrSource[xDim*(yDim-1)-xDim]+ptrSource[xDim*(yDim-1)-xDim+1]);
					  dataBuff /= 4.0f;
					  dataBuff = MAX(dataBuff, 0.0f);
					  dataBuff = MIN(dataBuff, 65535.0f);
					  *(ptrTarget+xDim*(yDim-1)) = (u16)dataBuff;
			   }
			   // bottom-right
			   {
					  float dataBuff = 0;
					  dataBuff = (float)((float)ptrSource[xDim*yDim-1]+ptrSource[xDim*yDim-1-1]+ptrSource[xDim*yDim-1-xDim]+ptrSource[xDim*yDim-1-1-xDim]);
					  dataBuff /= 4.0f;
					  dataBuff = MAX(dataBuff, 0.0f);
					  dataBuff = MIN(dataBuff, 65535.0f);
					  *(ptrTarget+xDim*yDim-1) = (u16)dataBuff;
			   }

			   // first row
			   {
					  for (int x=1;x<xDim-1;x++)
					  {
							  float dataBuff = 0;
							  dataBuff = (float)((float)ptrSource[x-1]+ptrSource[x]+ptrSource[x+1]+ptrSource[x+xDim-1]+ptrSource[x+xDim]+ptrSource[x+xDim+1]);
							  dataBuff /= 6.0f;
							  dataBuff = MAX(dataBuff, 0.0f);
							  dataBuff = MIN(dataBuff, 65535.0f);
							  *(ptrTarget+x) = (u16)dataBuff;
					  }
			   }

			   // last row
			   {
					  int y=yDim-1;
					  int yBuff = y*xDim;
					  for (int x=1;x<xDim-1;x++)
					  {
							  int xBuff = x+yBuff;
							  float dataBuff = 0;
							  dataBuff = (float)((float)ptrSource[xBuff-1]+ptrSource[xBuff]+ptrSource[xBuff+1]+ptrSource[xBuff-xDim-1]+ptrSource[xBuff-xDim]+ptrSource[xBuff-xDim+1]);
							  dataBuff /= 6.0f;
							  dataBuff = MAX(dataBuff, 0.0f);
							  dataBuff = MIN(dataBuff, 65535.0f);
							  *(ptrTarget+xBuff) = (u16)dataBuff;
					  }
			   }

			   // first column
			   {
					  for (int y=1;y<yDim-1;y++)
					  {
							  int nPos = y*xDim;
							  float dataBuff = 0;
							  dataBuff = (float)((float)ptrSource[nPos-xDim]+ptrSource[nPos]+ptrSource[nPos+xDim]+ptrSource[nPos-xDim+1]+ptrSource[nPos+1]+ptrSource[nPos+xDim+1]);
							  dataBuff /= 6.0f;
							  dataBuff = MAX(dataBuff, 0.0f);
							  dataBuff = MIN(dataBuff, 65535.0f);
							  *(ptrTarget+nPos) = (u16)dataBuff;
					  }
			   }

			   // last column
			   {
					  for (int y=1;y<yDim-1;y++)
					  {
							  int nPos = y*xDim+xDim-1;
							  float dataBuff = 0;
							  dataBuff = (float)((float)ptrSource[nPos-xDim]+ptrSource[nPos]+ptrSource[nPos+xDim]+ptrSource[nPos-xDim-1]+ptrSource[nPos-1]+ptrSource[nPos+xDim-1]);
							  dataBuff /= 6.0f;
							  dataBuff = MAX(dataBuff, 0.0f);
							  dataBuff = MIN(dataBuff, 65535.0f);
							  *(ptrTarget+nPos) = (u16)dataBuff;
					  }
			   }


	   }
	   else
	   {
			   for (int y=0;y<yDim;y++)
			   {
					  int yBuff = y*xDim;
					   for (int x=0;x<xDim;x++)
					  {
							  int xBuff = yBuff+x;
							  float divBuff = 0.0f;
							  float dataBuff = 0;
							  for (int dy=-radius;dy<=radius;dy++)
							  {
									 int dyBuff = (y+dy)*xDim;
									 if (((y+dy)>=0)&&((y+dy)<yDim))
									 {
											 for (int dx=-radius;dx<=radius;dx++)
											 {
													 int offsetBuff = dyBuff + (x+dx);
													 if (((x+dx)>=0)&&((x+dx)<xDim))
													 {
															dataBuff += *(ptrSource + offsetBuff);
															divBuff+=1.0f;
													 }
											 }
									 }
							  }
							  dataBuff /= divBuff;
							  dataBuff = MAX(dataBuff, 0.0f);
							  dataBuff = MIN(dataBuff, 65535.0f);
							  *(ptrTarget+xBuff) = (u16)dataBuff;
					  }
			   }
	   }

		return 0;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::GetAutoWindow(u16* ptrImage, int xDim, int yDim, int flatGamma, int postGamma, int maxValue, int blackOffset, int whiteOffset, int* window, int* level, int xStart, int xEnd, int yStart, int yEnd, int resizeFactor)
{
		int i;
		int minBuff;
		int maxBuff;
		u16 hist[65536];
		u16* ptrBuffer1 = NULL;
		u16* ptrBuffer2 = NULL;
		float calcBuffmin;
		float calcBuffmax;
		float flatGammaBuff;
		float postGammaBuff;
		int x, y;
		int xStartBuff, xEndBuff;
		int yStartBuff, yEndBuff;
		long offset;
		int sizeBuff;


	   ptrBuffer1 = (u16 *)malloc(sizeof(u16) * (xDim*yDim));
	   ptrBuffer2 = (u16 *)malloc(sizeof(u16) * (xDim*yDim));

		ShrinkImage(ptrImage, ptrBuffer1, xDim, yDim, resizeFactor);
		AverageFilter(ptrBuffer1, ptrBuffer2, xDim/resizeFactor, yDim/resizeFactor, 3);

		offset = 0;

		xStartBuff = xStart/resizeFactor+1;
		xEndBuff = xEnd/resizeFactor;
		yStartBuff = yStart/resizeFactor+1;
		yEndBuff = yEnd/resizeFactor;

		sizeBuff = xEndBuff-xStartBuff;
		sizeBuff /= 20;
		if (sizeBuff<20) sizeBuff = 20;
		xStartBuff += sizeBuff;
		xEndBuff -= sizeBuff;
		
		sizeBuff = yEndBuff-yStartBuff;
		sizeBuff /= 20;
		if (sizeBuff<20) sizeBuff = 20;
		yStartBuff += sizeBuff;
		yEndBuff -= sizeBuff;

		for (y=yStartBuff;y<yEndBuff;y++)
		{
			for (x=xStartBuff;x<xEndBuff;x++)
			{
				*(ptrBuffer1+offset) = *(ptrBuffer2+y*(xDim/resizeFactor)+x);
				offset++;
			}
		}
		GetHistogram(ptrBuffer1, hist, 65536, (xEndBuff-xStartBuff), (yEndBuff-yStartBuff));
		AvgHistogram(hist, 65536, 100);

		minBuff = 0;
		for (i=0;i<maxValue;i++)
		{
			if (hist[i]>0)
			{
				minBuff = i;
				i = maxValue;
			}
		}

		maxBuff = maxValue;
		for (i=maxValue;i>0;i--)
		{
			if (hist[i]>0)
			{
				maxBuff = i;
				i = 0;
			}
		}

		calcBuffmin = (float)minBuff;
		calcBuffmax = (float)maxBuff;
		
		
		flatGammaBuff = (float)flatGamma;
		flatGammaBuff /= 1000;
		postGammaBuff = (float)postGamma;
		postGammaBuff /= 1000;
		
		calcBuffmin /= (float)maxValue;
		calcBuffmin = pow(calcBuffmin, flatGammaBuff);
		calcBuffmin = pow(calcBuffmin, postGammaBuff);
		calcBuffmin *= (float)maxValue;
		
		calcBuffmax /= (float)maxValue;
		calcBuffmax = pow(calcBuffmax, flatGammaBuff);
		calcBuffmax = pow(calcBuffmax, postGammaBuff);
		calcBuffmax *= (float)maxValue;
		
		calcBuffmin -= calcBuffmin*((float)blackOffset/100);
		
		if (calcBuffmin<(0-maxValue)) 
			calcBuffmin = (float)(0-maxValue);

		calcBuffmax += calcBuffmax*((float)whiteOffset/100);
		if (calcBuffmax>maxValue*2) calcBuffmax = (float)maxValue*2;
		
		minBuff = (int)calcBuffmin;
		maxBuff = (int)calcBuffmax;
		
		*window = maxBuff - minBuff;
		*level = (maxBuff+minBuff)/2;
		
		free(ptrBuffer1);
		free(ptrBuffer2);

		return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::AutoROI(u16* _pImage, int _nWidth, int _nHeight, int _nUsingBits, int * _nWindowValue, int * _nLevelValue, int _nCompGamma, int _nPostGamma, int _nMaxValue)
{
           u16* projectedX = NULL;
           u16* projectedY = NULL;
		   int colliX1, colliX2, colliY1, colliY2;
		   const int xExclusion = _nWidth/46;
		   const int yExclusion = _nHeight/46;
		   const int xSearchRange = _nWidth/3;
		   const int ySearchRange = _nHeight/3;


		   projectedX = (u16 *)malloc(sizeof(u16) * (_nWidth));
		   projectedY = (u16 *)malloc(sizeof(u16) * (_nHeight));

		   // Auto collimator Start
		   //
           GetProjectionData(_pImage, projectedX, projectedY, _nWidth, _nHeight);
           AvgHistogram(projectedX, _nWidth, 10);
           AvgHistogram(projectedY, _nHeight, 10);
           GetPeakHistogram(projectedX, _nWidth, 50);
		   GetPeakHistogram(projectedY, _nHeight, 50);

           colliX1 = GetX1ColliPosition2(projectedX, xExclusion, xSearchRange, _nWidth, 5);                    // start > span/2
		   colliX2 = GetX2ColliPosition2(projectedX, _nWidth-xSearchRange, _nWidth-xExclusion, _nWidth, 5);                    // imagewidth-end > span/2
           colliY1 = GetY1ColliPosition2(projectedY, yExclusion, ySearchRange, _nHeight, 5);                    // start > span/2
           colliY2 = GetY2ColliPosition2(projectedY, _nHeight-ySearchRange, _nHeight-yExclusion, _nHeight, 5);                  // imageheight-end > span/2

		   free(projectedX);
		   free(projectedY);

		   if (colliX1<0) colliX1 = 0;
           if (colliX2>_nWidth) colliX2 = _nWidth;

           if(colliX2 < colliX1)
                     colliX2 = _nWidth;


		   if (colliY1<0) colliY1 = 0;
           if (colliY2>_nHeight) colliY2 = _nHeight;

		   if(colliY2 < colliY1)
					 colliY2 = _nHeight;

           int autoResizeFactor = 1;
           int autoBlackOffset = 0;
           int autoWhiteOffset = 0;
		   GetAutoWindow(_pImage, _nWidth, _nHeight, _nCompGamma, _nPostGamma, _nMaxValue, autoBlackOffset, autoWhiteOffset,
                     _nWindowValue, _nLevelValue, colliX1, colliX2, colliY1, colliY2, autoResizeFactor);

		   return true;

}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::Mapping(u16 * pImageBuf, int nWidth, int nHeight, long srcLow, long srcHigh, long desLow, long desHigh)
{       
        double m = (desHigh - desLow) /(double)((srcHigh -srcLow));  
        int nPixelCount = nWidth*nHeight;

		int iter=0;
		int nTempindex;
		int nLine;
		u16* pTempImage = (u16*)malloc(sizeof(u16) * nPixelCount);
        int * pTemp = (int *)malloc(sizeof(int) * (srcHigh-srcLow+1));

        for (iter=0; iter <=srcHigh-srcLow; iter++)      
               pTemp[iter] = m*iter + desLow;        

        int index = 0;
        int iPixel = 0;

		memcpy(pTempImage, pImageBuf, nPixelCount*2);
        for(iter=0;iter < nPixelCount;iter++)
        {
				nLine = (iter / nWidth) + 1; 
				nTempindex = iter % nWidth;
				nLine = nHeight - nLine; 
				nTempindex = (nWidth * nLine) + nTempindex;  


               iPixel = pTempImage[iter]-srcLow;
               if (iPixel < 0)
			   {
                       pTempImage[index] = desLow;  
			   }
               else
               {
                       if (iPixel>srcHigh-srcLow)
                              pTempImage[index] = desHigh;         
                       else
                              pTempImage[index] = pTemp[iPixel];
               }       

			   pImageBuf[nTempindex] = pTempImage[index];
			   index++;
        }       
		free(pTemp);
		free(pTempImage);
}


/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::SaveMono14ToBMP(const s8* i_p_file_name, u8* pImage )
{
	BITMAPINFO* pBmpInfo;
	FILE * fp;
	//Gets the updated values for the structure
	u8 *tempbuffer;
	bool b_result = false;
	u32 n_image_size;
	
	
	pBmpInfo = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));
	
	memset((u8 *)pBmpInfo,0,sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));
	pBmpInfo->bmiHeader.biSize            = sizeof( BITMAPINFOHEADER );
	pBmpInfo->bmiHeader.biWidth           = m_n_quarter_width;
	pBmpInfo->bmiHeader.biHeight          = m_n_quarter_height;              ///*bMirror ? */nHeight/* : -(int)m_nImageHeight*/;
	pBmpInfo->bmiHeader.biPlanes          = 1;
	pBmpInfo->bmiHeader.biBitCount        = 24;
	pBmpInfo->bmiHeader.biClrUsed         = 0;
	pBmpInfo->bmiHeader.biClrImportant    = 0;
	pBmpInfo->bmiHeader.biCompression = 0;
	
	n_image_size = m_n_quarter_width * m_n_quarter_height;
	
	tempbuffer = (u8 *) malloc( (m_n_quarter_width * m_n_quarter_height * 3) );
	
	
	memset( tempbuffer,0, (m_n_quarter_width * m_n_quarter_height * 3) );                                                                                                   //16Bit
	
	
	for( u32 i = 0; i < n_image_size;i ++)
	{
		u8 data;

		//data = (((u16*)pImage)[ n_image_size - 1 - i ] >> 6);
		data = (((u16*)pImage)[ i ] >> 6);
		tempbuffer[i * 3] = data;
		tempbuffer[i * 3 + 1] = data;
		tempbuffer[i * 3 + 2] = data;
	}
	
	u32 cbImageSize = (m_n_quarter_width * m_n_quarter_height * 24) / 8;
	
	u32 cbHeaderSize = pBmpInfo->bmiHeader.biSize + pBmpInfo->bmiHeader.biClrUsed * sizeof(RGBQUAD);
	
	BITMAPFILEHEADER bfh;
	
	memset(&bfh,0,sizeof(BITMAPFILEHEADER));
	
	
	bfh.bfType         = 'B'+('M'<<8);
	bfh.bfOffBits      = sizeof(BITMAPFILEHEADER) + cbHeaderSize;
	bfh.bfSize         = sizeof(BITMAPFILEHEADER) + cbHeaderSize + cbImageSize;
	
	BITMAPINFO* bi;
	
	bi = (BITMAPINFO *)malloc(cbHeaderSize);
	
	memcpy( bi, pBmpInfo, cbHeaderSize );
	
	fp = fopen(i_p_file_name,"wb");

	if(fp)
	{
		fwrite((char*)&bfh,1,sizeof(bfh),fp);
		fwrite((char*)bi,1,cbHeaderSize,fp);
		fwrite((char*)tempbuffer,1,cbImageSize,fp);
	
		fclose(fp);
		
		b_result = true;
	}
	
	print_log(INFO,1,"[%s] preview file is %s\n", m_p_log_id, i_p_file_name );
	
	free(bi);
	//free(pbImageData);
	free(tempbuffer);
	free(pBmpInfo);

	return b_result;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::apply_gain(u16* image)
{
	int i;
	int nPixelCount = m_n_quarter_width * m_n_quarter_height;
	float fMaxPixelValue = (float)(MAXVALUE_FROM_BIT(16));
	
	if( m_b_gain == false
		|| m_p_gain == NULL )
	{
		print_log(INFO,1,"[%s] Gain is not applied.\n", m_p_log_id);
		return;
	}

	for(i=0; i< nPixelCount ; i++)
	{
		if((float)m_p_gain[i] == 0.0f)
		{
			image[i] = 0;
			continue;
		}
		float fnewValue = MIN(MAX((float)(image[i]) * (float)(m_p_gain[i]), 0.0f), fMaxPixelValue);
		image[i] = (u16)(fnewValue);
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::window_level_lut_image(unsigned short* pIn, unsigned short* pOut, unsigned short *pLut, unsigned int nInUsingBits, unsigned int nOutUsingBits, unsigned int nLutUsingBits, unsigned int nInBytesPerLine, unsigned int nOutBytesPerLine, unsigned int nWidth, unsigned int nHeight)
{
	if(!pIn)
	{
		return false;
	}

	if(!pOut)
	{
		return false;
	}

	if(!pLut)
	{
		return false;
	}

	if(nInUsingBits<1)
	{
		return false;
	}

	if(nOutUsingBits<1)
	{
		return false;
	}

	if(nLutUsingBits<1)
	{
		return false;
	}

	if(nInUsingBits>nLutUsingBits)      //LUT 12bit 옜 옜?옜.
	{
		return false;
	}

	if(nInBytesPerLine<1)
	{
		return false;
	}

	if(nOutBytesPerLine<1)
	{
		return false;
	}

	if(nWidth<1)
	{
		return false;
	}

	if(nHeight<1)
	{
		return false;
	}


	unsigned short **arImageIn = NULL;
	arImageIn = (unsigned short**)malloc(nHeight * sizeof(unsigned short*));
	if(!arImageIn)
	{
		return false;
	}

	unsigned short **arImageOut = NULL;
	arImageOut = (unsigned short**)malloc(nHeight * sizeof(unsigned short*));
	if(!arImageOut)
	{
		if(arImageIn)
		{
			free(arImageOut);
			arImageOut = NULL;
		}

		return false;
	}

	arImageIn[0] = (unsigned short*)pIn;
	for(int i=1; i<(int)nHeight; i++) 
	{
		unsigned char *pBYTE = (unsigned char*)arImageIn[i-1];
		pBYTE += nInBytesPerLine;
		arImageIn[i] = (unsigned short*)pBYTE;
	}

	arImageOut[0] = (unsigned short*)pOut;
	for(int i=1; i<(int)nHeight; i++) 
	{
		unsigned char *pBYTE = (unsigned char*)arImageOut[i-1];
		pBYTE += nOutBytesPerLine;
		arImageOut[i] = (unsigned short*)pBYTE;
	}

	int i,j;
	int row, col;
	int pixel_count;
	int size_quarter;
	pixel_count        = nWidth*nHeight;
	size_quarter = (int)(pixel_count/4);

	for(i=0; i<size_quarter; i++)
	{
		j = i<<2;
		row = (int)(j/nWidth);
		col = (int)(j%nWidth);

		arImageOut[row][col+0] = (unsigned short)(pLut[(int)(arImageIn[row][col+0])]);
		arImageOut[row][col+1] = (unsigned short)(pLut[(int)(arImageIn[row][col+1])]);
		arImageOut[row][col+2] = (unsigned short)(pLut[(int)(arImageIn[row][col+2])]);
		arImageOut[row][col+3] = (unsigned short)(pLut[(int)(arImageIn[row][col+3])]);
	}

	if(arImageIn)
	{
		free(arImageIn);
		arImageIn = NULL;
	}

	if(arImageOut)
	{
		free(arImageOut);
		arImageOut = NULL;
	}

	return true;
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::process( u16* i_p_image, bool i_b_invert )
{
	int window,level;
	int	w1,w2;

//	// apply gain
//	apply_gain( i_p_image );
//
//	defect_correction( i_p_image );
	
	// make 14bit
	u32 i;
	for( i = 0; i < (m_n_quarter_width * m_n_quarter_height); i++ )
	{
		i_p_image[i] = i_p_image[i] >> 2;
	}
	//ROI
	AutoROI( i_p_image, m_n_quarter_width, m_n_quarter_height, 14, &window, &level, 1000, 1000, 14500 );
	
	w1 = level - window/2;
	w2 = level + window/2;

	Mapping(i_p_image,m_n_quarter_width, m_n_quarter_height,w1,w2,0,16383);

	if( i_b_invert == true )
	{
		print_log(INFO,1,"[%s] Negative image.\n", m_p_log_id);
		window_level_lut_image(i_p_image, i_p_image, s_p_lut, 14, 14, 14, (m_n_quarter_width * 2), (m_n_quarter_width * 2), m_n_quarter_width, m_n_quarter_height);
	}
	else
	{
		print_log(INFO,1,"[%s] Positive image.\n", m_p_log_id);
	}	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
bool CImageProcess::defect_load( u32 i_n_total_count, FILE* i_p_file )
{
	u32 i;
	
	if( i_p_file == NULL )
	{
		print_log(ERROR,1,"[%s] There is no defect data (file is NULL)\n", m_p_log_id);
		return false;
	}

	m_b_defect = false;
	safe_free(m_p_defect_info_t);
	
	if(i_n_total_count > 0)
	{
		m_p_defect_info_t = (defect_pixel_info_t*)malloc(sizeof(defect_pixel_info_t) * i_n_total_count);
	
		if( m_p_defect_info_t == NULL )
		{
			print_log(ERROR,1,"[%s] defect malloc failed\n", m_p_log_id);
			return false;
		}
		
		for( i = 0; i < i_n_total_count; i++ )
		{
			fread( &m_p_defect_info_t[i], 1, sizeof(defect_pixel_info_t), i_p_file );
		}
	}
	
	m_n_defect_total_count = i_n_total_count;

	m_b_defect = true;
	
	return true;
	
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::defect_correction_pixel( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info )
{
	u16	n_pos_x = i_p_defect_info->n_position_x;
    u16	n_pos_y = i_p_defect_info->n_position_y;
    
    // {y,x}
	signed char p_pos_info[48][2] = { 
		{0,-1},{0,1},{-1,0},{1,0},{-1,-1},{1,-1},{-1,1},{1,1},
		{0,-2},{0,2},{-2,0},{2,0},{-2,-1},{2,-1},{-2,1},{2,1},
		{-1,-2},{1,-2},{-1,2},{1,2},{-2,-2},{2,-2},{-2,2},{2,2},
		{0,-3},{0,3},{-3,0},{3,0},{-3,-1},{3,-1},{-3,1},{3,1},
		{-1,-3},{1,-3},{-1,3},{1,3},{-3,-2},{3,-2},{-3,2},{3,2},
		{-2,-3},{2,-3},{-2,3},{2,3},{-3,-3},{3,-3},{-3,3},{3,3}
	};
	s32 n_sum = 0;
	u32 n_jdx;
	u8 b_pixels[4];
	*(u32*)b_pixels = i_p_defect_info->n_src_info;
	
	for ( n_jdx = 0; n_jdx < 4 ; n_jdx++)
	{
		if(b_pixels[n_jdx] >= 48)
			break;
		
		
		n_sum += i_p_buffer[(n_pos_y + p_pos_info[b_pixels[n_jdx]][1]) * m_n_width
									+p_pos_info[b_pixels[n_jdx]][0] + n_pos_x];
		
		//printf("ref_pixel %d %d\n",p_pos_info[b_pixels[n_jdx]][1],p_pos_info[b_pixels[n_jdx]][0] );
	}

	if( n_jdx > 0 )
	{
		i_p_buffer[n_pos_y * m_n_width + n_pos_x] = (u16)(n_sum/n_jdx);
	}
#if 0
	else
	{
		print_log(ERROR,1,"[%s] cluster defect(%d, %d)\n", m_p_log_id, n_pos_x, n_pos_y);
	}
#endif
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::defect_correction_hline( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info )
{
	u16	n_pos_x = i_p_defect_info->n_position_x;
    u16	n_pos_y = i_p_defect_info->n_position_y;
    
	u16 w_pixels[2];
    			
	*(u32*)w_pixels = i_p_defect_info->n_src_info;

	i_p_buffer[n_pos_y * m_n_width + n_pos_x] = (u16)(
	(i_p_buffer[(n_pos_y - w_pixels[0]) * m_n_width + n_pos_x ] * w_pixels[1] + 
	i_p_buffer[(n_pos_y + w_pixels[1]) * m_n_width + n_pos_x ] * w_pixels[0])/
	(w_pixels[0] + w_pixels[1]));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::defect_correction_vline( u16* i_p_buffer, defect_pixel_info_t* i_p_defect_info )
{
	u16	n_pos_x = i_p_defect_info->n_position_x;
    u16	n_pos_y = i_p_defect_info->n_position_y;
    
    u16 w_pixels[2];
    			
	*(u32*)w_pixels = i_p_defect_info->n_src_info;
	
	i_p_buffer[n_pos_y * m_n_width + n_pos_x] = (u16)(
    			(i_p_buffer[n_pos_y * m_n_width + n_pos_x - w_pixels[0]] * w_pixels[1] + 
    			i_p_buffer[n_pos_y * m_n_width + n_pos_x + w_pixels[1]] * w_pixels[0])/
    			(w_pixels[0] + w_pixels[1]));
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::defect_correction( u16* i_p_buffer, u32 i_n_cur_line_index )
{
	if( m_p_defect_info_t == NULL
		|| m_b_defect == false )
	{
		//print_log( INFO, 1, "[%s] Defect is not applied.\n", m_p_log_id );
		return;
	}
	
	for( ; m_n_idx < m_n_defect_total_count && m_p_defect_info_t[m_n_idx].w_available_line <= i_n_cur_line_index; m_n_idx++ )
	{			
		if( m_p_defect_info_t[m_n_idx].b_defect_type == eDEFECT_PIXEL )
    	{
    		defect_correction_pixel( i_p_buffer, &m_p_defect_info_t[m_n_idx] );
    	}
    	else if( m_p_defect_info_t[m_n_idx].b_defect_type == eDEFECT_LINE_H )
    	{
    		defect_correction_hline( i_p_buffer, &m_p_defect_info_t[m_n_idx] );
    	}
    	else if( m_p_defect_info_t[m_n_idx].b_defect_type == eDEFECT_LINE_V )
    	{
    		defect_correction_vline( i_p_buffer, &m_p_defect_info_t[m_n_idx] );
    	}
	}
}

/******************************************************************************/
/**
 * @brief   
 * @param   
 * @return  
 * @note    
*******************************************************************************/
void CImageProcess::defect_print(void)
{	
	u32 i;
	FILE* p_file;
	
	p_file = fopen( "/tmp/defect.list", "w");
	
	fprintf( p_file, "total count: %d\n", m_n_defect_total_count );

	if( m_n_defect_total_count > 0 )	 
	{
		for( i = 0; i < m_n_defect_total_count; i++ )
		{
			fprintf( p_file, "[%08d] type: %d, line: %d, src info: 0x%08X, pos x: %d, pos y: %d\n", \
							i, m_p_defect_info_t[i].b_defect_type, m_p_defect_info_t[i].w_available_line,
							m_p_defect_info_t[i].n_src_info, m_p_defect_info_t[i].n_position_x, \
							m_p_defect_info_t[i].n_position_y );
		}
	}
	
	fclose(p_file);
}                                                                              
