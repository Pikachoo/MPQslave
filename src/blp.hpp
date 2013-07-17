#ifndef BLP_HPP_
#define BLP_HPP_

// source: github.com/Kanma/BLPConverter

#include <iostream>
#include <stdint.h>
#include <memory.h>
#include <stdio.h>
#include <string>

#include <squish.h>		// -lsquish
#include <FreeImage.h>	// -lfreeimage

#include "IO.hpp"

struct tBGRAPixel
{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
};

// A description of the BLP1 format can be found in the file doc/MagosBlpFormat.txt
struct tBLP1Header
{
		uint8_t magic[4];		// Always 'BLP1'
		uint32_t type;			// 0: JPEG, 1: palette
		uint32_t flags;			// 8: Alpha
		uint32_t width;			// In pixels, power-of-two
		uint32_t height;
		uint32_t alphaEncoding;	// 3, 4: Alpha list, 5: Alpha from palette
		uint32_t flags2;		// Unused
		uint32_t offsets[16];
		uint32_t lengths[16];
};

// Additional informations about a BLP1 file
struct tBLP1Infos
{
		uint8_t nbMipLevels;				// The number of mip levels

		union
		{
				tBGRAPixel palette[256];	// 256 BGRA colors

				struct
				{
						uint32_t headerSize;
						uint8_t* header;	// Shared between all mipmap levels
				} jpeg;
		};
};

// A description of the BLP2 format can be found on Wikipedia: http://en.wikipedia.org/wiki/.BLP
struct tBLP2Header
{
		uint8_t magic[4];				// Always 'BLP2'
		uint32_t type;					// 0: JPEG, 1: see encoding
		uint8_t encoding;				// 1: Uncompressed, 2: DXT compression
		uint8_t alphaDepth;				// 0, 1, 4 or 8 bits
		uint8_t alphaEncoding;			// 0: DXT1, 1: DXT3, 7: DXT5

		union
		{
				uint8_t hasMipLevels;	// In BLP file: 0 or 1
				uint8_t nbMipLevels;	// For convenience, replaced with the number of mip levels
		};

		uint32_t width;					// In pixels, power-of-two
		uint32_t height;
		uint32_t offsets[16];
		uint32_t lengths[16];
		tBGRAPixel palette[256];		// 256 BGRA colors
};

// Internal representation of any BLP header
struct tInternalBLPInfos
{
		uint8_t version;				// 1 or 2

		union
		{
				struct
				{
						tBLP1Header header;
						tBLP1Infos infos;
				} blp1;

				tBLP2Header blp2;
		};
};

// Opaque type representing a BLP file
typedef void* tBLPInfos;

enum tBLPEncoding
{
	BLP_ENCODING_UNCOMPRESSED = 1, BLP_ENCODING_DXT = 2,
};

enum tBLPAlphaDepth
{
	BLP_ALPHA_DEPTH_0 = 0, BLP_ALPHA_DEPTH_1 = 1, BLP_ALPHA_DEPTH_4 = 4, BLP_ALPHA_DEPTH_8 = 8,
};

enum tBLPAlphaEncoding
{
	BLP_ALPHA_ENCODING_DXT1 = 0, BLP_ALPHA_ENCODING_DXT3 = 1, BLP_ALPHA_ENCODING_DXT5 = 7,
};

enum tBLPFormat
{
	BLP_FORMAT_JPEG = 0,

	BLP_FORMAT_PALETTED_NO_ALPHA = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_0 << 8),
	BLP_FORMAT_PALETTED_ALPHA_1 = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_1 << 8),
	BLP_FORMAT_PALETTED_ALPHA_8 = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_8 << 8),

	BLP_FORMAT_DXT1_NO_ALPHA = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_0 << 8) | BLP_ALPHA_ENCODING_DXT1,
	BLP_FORMAT_DXT1_ALPHA_1 = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_1 << 8) | BLP_ALPHA_ENCODING_DXT1,
	BLP_FORMAT_DXT3_ALPHA_4 = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_4 << 8) | BLP_ALPHA_ENCODING_DXT3,
	BLP_FORMAT_DXT3_ALPHA_8 = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_8 << 8) | BLP_ALPHA_ENCODING_DXT3,
	BLP_FORMAT_DXT5_ALPHA_8 = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_8 << 8) | BLP_ALPHA_ENCODING_DXT5,
};

namespace MPQs
{
	class blp
	{
		private:
			bool _debug;

			tBLPInfos process_file(FILE *pFile);
			void release(tBLPInfos blpInfos);

			uint8_t version(tBLPInfos blpInfos);
			tBLPFormat format(tBLPInfos blpInfos);

			unsigned int width(tBLPInfos blpInfos, unsigned int mipLevel = 0);
			unsigned int height(tBLPInfos blpInfos, unsigned int mipLevel = 0);
			unsigned int nb_mip_levels(tBLPInfos blpInfos);

			tBGRAPixel* convert(FILE* pFile, tBLPInfos blpInfos, unsigned int mipLevel = 0);

			std::string as_string(tBLPFormat format);

		public:
			blp(bool debug = true);

			int parse(const std::string &blp_file, const std::string &out_format = "png");
	};

	class blp1
	{
		public:
			static tBGRAPixel* convert_jpeg(uint8_t* pSrc, tBLP1Infos* pInfos, uint32_t size);
			static tBGRAPixel* convert_paletted_alpha(uint8_t* pSrc, tBLP1Infos* pInfos, unsigned int width, unsigned int height);
			static tBGRAPixel* convert_paletted_no_alpha(uint8_t* pSrc, tBLP1Infos* pInfos, unsigned int width, unsigned int height);
			static tBGRAPixel* convert_paletted_separated_alpha(uint8_t* pSrc, tBLP1Infos* pInfos, unsigned int width, unsigned int height, bool invertAlpha);
	};

	class blp2
	{
		public:
			static tBGRAPixel* convert_paletted_no_alpha(uint8_t* pSrc, tBLP2Header* pHeader, unsigned int width, unsigned int height);
			static tBGRAPixel* convert_paletted_alpha1(uint8_t* pSrc, tBLP2Header* pHeader, unsigned int width, unsigned int height);
			static tBGRAPixel* convert_paletted_alpha8(uint8_t* pSrc, tBLP2Header* pHeader, unsigned int width, unsigned int height);
			static tBGRAPixel* convert_dxt(uint8_t* pSrc, tBLP2Header* pHeader, unsigned int width, unsigned int height, int flags);
	};
}

#endif /* BLP_HPP_ */
