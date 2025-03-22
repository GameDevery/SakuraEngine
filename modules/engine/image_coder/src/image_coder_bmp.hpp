#pragma once
#include "SkrBase/config.h"
#include "image_coder_base.hpp"

namespace skr
{

struct FBitmapFileHeader;
struct FBitmapInfoHeader;

/*
struct SKR_IMAGE_CODER_API BMPImageEncoder : public BaseImageEncoder
{
public:
    BMPImageEncoder() SKR_NOEXCEPT;
    ~BMPImageEncoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;

    bool encode() SKR_NOEXCEPT final;

    bool bHalfHeight;
};
*/

struct SKR_IMAGE_CODER_API BMPImageDecoder : public BaseImageDecoder
{
public:
    BMPImageDecoder() SKR_NOEXCEPT;
    ~BMPImageDecoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;
    bool initialize(const uint8_t* data, uint64_t size) SKR_NOEXCEPT final;
    bool decode(EImageCoderColorFormat format, uint32_t bit_depth) SKR_NOEXCEPT final;

    bool load_bmp_header() SKR_NOEXCEPT;

    /** BMP as a sub-format of ICO stores its height as half their actual size */
    bool bHalfHeight = false;
    const FBitmapInfoHeader* bmhdr = nullptr;
    const FBitmapFileHeader * bmfh = nullptr;

};

// Bitmap compression types.
enum EBitmapCompression
{
	BCBI_RGB            = 0,
	BCBI_RLE8           = 1,
	BCBI_RLE4           = 2,
	BCBI_BITFIELDS      = 3,
	BCBI_JPEG           = 4,
	BCBI_PNG            = 5,
	BCBI_ALPHABITFIELDS = 6,
};

// Bitmap info header versions.
enum class EBitmapHeaderVersion : uint8_t
{
	BHV_BITMAPINFOHEADER   = 0,
	BHV_BITMAPV2INFOHEADER = 1,
	BHV_BITMAPV3INFOHEADER = 2,
	BHV_BITMAPV4HEADER     = 3,
	BHV_BITMAPV5HEADER     = 4,
	BHV_INVALID = 0xFF
};

// Color space type of the bitmap, property introduced in Bitmap header version 4.
enum class EBitmapCSType : uint32_t
{
	BCST_BLCS_CALIBRATED_RGB     = 0x00000000,
	BCST_LCS_sRGB                = 0x73524742,
	BCST_LCS_WINDOWS_COLOR_SPACE = 0x57696E20,
	BCST_PROFILE_LINKED          = 0x4C494E4B,
	BCST_PROFILE_EMBEDDED        = 0x4D424544,
};

// .BMP file header.
#pragma pack(push,1)
struct FBitmapFileHeader
{
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};
#pragma pack(pop)

// .BMP subheader.
#pragma pack(push,1)
struct FBitmapInfoHeader
{
	uint32_t biSize;
	uint32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	
public:
	EBitmapHeaderVersion GetHeaderVersion() const
	{
		// Since there is no field indicating the header version of the bitmap in the FileHeader,
		// the only way to know the format version is to check the header size.
		//
		// note that Adobe (incorrectly) writes biSize as 40 and then BMFH bfOffBits will be 52 or 56
		//	so this code does not detect those variant headers
		switch (biSize)
		{
			case 40:
				return EBitmapHeaderVersion::BHV_BITMAPINFOHEADER;
			case 52:
				// + RGB 32 bit masks
				return EBitmapHeaderVersion::BHV_BITMAPV2INFOHEADER;
			case 56:
				// + RGBA 32 bit masks
				return EBitmapHeaderVersion::BHV_BITMAPV3INFOHEADER;
			case 108:
				return EBitmapHeaderVersion::BHV_BITMAPV4HEADER;
			case 124:
				return EBitmapHeaderVersion::BHV_BITMAPV5HEADER;
			default:
				return EBitmapHeaderVersion::BHV_INVALID;
		}
	}
};
#pragma pack(pop)


// .BMP subheader V4
#pragma pack(push,1)
struct FBitmapInfoHeaderV4
{
	uint32_t biSize;
	uint32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	uint32_t biRedMask;
	uint32_t biGreenMask;
	uint32_t biBlueMask;
	uint32_t biAlphaMask;
	uint32_t biCSType;
	int32_t biEndPointRedX;
	int32_t biEndPointRedY;
	int32_t bibiEndPointRedZ;
	int32_t bibiEndPointGreenX;
	int32_t biEndPointGreenY;
	int32_t biEndPointGreenZ;
	int32_t biEndPointBlueX;
	int32_t biEndPointBlueY;
	int32_t biEndPointBlueZ;
	uint32_t biGammaRed;
	uint32_t biGammaGreen;
	uint32_t biGammaBlue;
};
#pragma pack(pop)


#pragma pack(push, 1)
//Used by InfoHeaders pre-version 4, a structure that is declared after the FBitmapInfoHeader.
struct FBmiColorsMask
{
	// RGBA, in header pre-version 4, Alpha was only used as padding.
	uint32_t RGBAMask[4];
};
#pragma pack(pop)

}