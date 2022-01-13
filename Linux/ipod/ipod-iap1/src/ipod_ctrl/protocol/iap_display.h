#ifndef IAP_DIPLAY_H
#define IAP_DIPLAY_H

#include <adit_typedef.h>

#include "iap_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* defines                                                                   */
/* ========================================================================= */

/* set display image ------------------------------------------------------- */

/* 5 because of Lingo, Command and Descriptor index */ 
#define IPOD_IMG_UPLOAD_ADDITIONAL_BYTES    5  
/* 32 Bit alignment for image data */
#define IPOD_IMG_UPLOAD_BYTE_ALIGN          4  
#define IPOD_IMG_UPLOAD_WIDTH_SIZE          4
#define IPOD_IMG_UPLOAD_HEIGHT_SIZE         4
#define IPOD_IMG_UPLOAD_IMG_DATA_START      16
#define IPOD_IMG_UPLOAD_FILE_OPEN_MODE      "rb"
#define IPOD_MAX_BYTES_PER_ROW_BLOCK        480
#define IPOD_IMAGE_BYTE_ALIGN               2

#define IPOD_BITS_PER_SHORT                 16
#define IPOD_BITS_PER_BYTE                  8
#define IPOD_BITSHIFT_2 2
#define IPOD_BITSHIFT_3 3
#define IPOD_BITSHIFT_5 5
#define IPOD_BITSHIFT_11 11
#define IPOD_RGBMASK_LOW   0x000000FF
#define IPOD_RGBMASK_MID   0x0000FF00
#define IPOD_RGBMASK_HIGH  0x00FF0000

#define IPOD_BMP_HEADER_POS_18   18 /* mark the image-width    */
#define IPOD_BMP_HEADER_POS_20   20 /* mark the image-height   */                     
#define IPOD_BMP_HEADER_POS_22   22 /* mark the image-height (monochrome bmp */                     
#define IPOD_BMP_HEADER_POS_28   28 /* mark the bits per pixel */      
#define IPOD_BMP_BIT_PER_PIXEL   24

#define IPOD_BMP_POINTER_TO_PIXEL_DATA      10

#define IPOD_SET_ROWSIZE2 2
#define IPOD_SET_ROWSIZE31 31
#define IPOD_SET_ROWSIZE32 32
#define IPOD_SET_ROWSIZE4 4

/* byte sizes in iPod response --------------------------------------------- */
#define IPOD_DISPLAY_IMG_LIMIT_SIZE         5
#define IPOD_ARTWORK_FORMAT_SIZE            7
#define IPOD_MAX_ARTWORK_FORMATS            32
#define IPOD_TRACK_ARTWORK_COUNT_DATA_SIZE  4
#define IPOD_RGB_NUMBER 3
#define IPOD_BYTE_LOOP 6
#define IPOD_BYTE_REDUCE 2
#define IPOD_WIN_BMP0 0x00
#define IPOD_WIN_BMP1 0x01
#define IPOD_WIN_BMP2 0x02
#define IPOD_WIN_BMP3 0x03
#define IPOD_READ_DATA_NUMBER 1
/* ========================================================================= */
/* function prototypes                                                      */
/* ========================================================================= */
S32 iPodGetArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, 
                          U16* resultCount);
S32 iPodGetMonoDisplayImageLimits(U32 iPodID, U16* width, 
                                  U16* height, 
                                  U8* pixelFormat);
S32 iPodGetColorDisplayImageLimits(U32 iPodID, IPOD_DISPLAY_IMAGE_LIMITS* resultBuf, 
                                   U16* resultCount);
S32 iPodSetDisplayImage(U32 iPodID, const U8* image, 
                        IPOD_IMAGE_TYPE imageType);
S32 iPodSetDisplayImageBMP(U32 iPodID, const U8* bmpImage);
S32 iPodSetDisplayImageMemory(U32 iPodID, const U8* bmpImage);

S32 iPodGetTrackArtworkData(U32 iPodID, U32 trackIndex,
                            U16 formatId,
                            U32 timeOffset,
                            const IPOD_CB_GET_ARTWORK callback);

S32 iPodGetTrackArtworkTimes(U32 iPodID, U32 trackIndex,
                             U16 formatId,
                             U16 artworkIndex,
                             U16 artworkCount,
                             U16* resultCount,
                             U32* buffer);
                             
S32 iPodGetTypeOfTrackArtworkData(U32 iPodID, IPOD_TRACK_TYPE type, U64 trackIndex,
                            U16 formatId,
                            U32 timeOffset,
                            const IPOD_CB_GET_ARTWORK callback);

S32 iPodGetTypeOfTrackArtworkTimes(U32 iPodID, IPOD_TRACK_TYPE type, 
                                    U32  trackIndex,
                                    U16  formatId,
                                    U16  artworkIndex,
                                    U16  artworkCount,
                                    U16 *resultCount,
                                    U32 *buffer);


S32 setImage(U32 iPodID, U32 imageWidth, U32 imageHeight, U16 *image, IPOD_IMAGE_TYPE imageType);
//S32 iPodSetDisplayImageMemory(U8 *image, U32 imageWidth, U32 imageHeight, IPOD_IMAGE_TYPE imageType);
S32 iPodSetDisplayImageBMPStoredMonochrom(U32 iPodID, const U8* bmpImage);

U32 iPodReadU32(FILE *file);
U16 iPodReadU16(FILE *file);
S32 iPodReadS32(FILE *file);
S16 iPodReadS16(FILE *file);
void iPodRGBToRGB565(U8 r,
                     U8 g,
                     U8 b,
                     U16 *rgb565);
                     
void iPodReadAndConvertBMPMemory(const U8* file,
                               U16** rgb565Buffer,
                               U32*  width,
                               U32*  height);

void iPodReadAndConvertBMPFile(FILE* file,
                               U16** rgb565Buffer,
                               U32*  width,
                               U32*  height);

#ifdef __cplusplus
}
#endif

#endif /* IAP_DIPLAY_H */
