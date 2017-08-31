#include <sdw.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

struct SGlyph
{
	u32 Height;
	u32 Width;
	n32 Left;
	n32 Top;
	u32 Advance;
	vector<u8> Data;
};

int UMain(int argc, UChar* argv[])
{
	if (argc != 13)
	{
		return 1;
	}
	n32 nFaceIndex = SToN32(argv[4]);
	u32 uPixelSize = SToU32(argv[5]);
	bool bMono = UCscmp(argv[6], USTR("0")) != 0;
	f32 fBold = SToF32(argv[7]);
	n32 nPaddingLeft = SToN32(argv[8]);
	n32 nPaddingRight = SToN32(argv[9]);
	n32 nPaddingTop = SToN32(argv[10]);
	n32 nPaddingBottom = SToN32(argv[11]);
	n32 nBase = SToN32(argv[12]);
	FILE* fp = UFopen(argv[1], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uTxtSize = ftell(fp);
	if (uTxtSize % 2 != 0)
	{
		fclose(fp);
		return 1;
	}
	uTxtSize /= 2;
	fseek(fp, 0, SEEK_SET);
	Char16_t* pTemp = new Char16_t[uTxtSize + 1];
	fread(pTemp, 2, uTxtSize, fp);
	fclose(fp);
	if (pTemp[0] != 0xFEFF)
	{
		delete[] pTemp;
		return 1;
	}
	pTemp[uTxtSize] = 0;
	U16String sTxt = pTemp + 1;
	delete[] pTemp;
	vector<Char16_t> vCharset;
	for (u32 i = 0; i < static_cast<u32>(sTxt.size()); i++)
	{
		Char16_t uUnicode = sTxt[i];
		if (uUnicode >= 0x20)
		{
			vCharset.push_back(uUnicode);
		}
	}
	vCharset.resize(static_cast<u32>(Align(vCharset.size(), 16)));
	fp = UFopen(argv[3], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uFontFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	vector<u8> vFontFile(uFontFileSize);
	fread(&*vFontFile.begin(), 1, uFontFileSize, fp);
	fclose(fp);
	FT_Library pLibrary = nullptr;
	FT_Error nError = FT_Init_FreeType(&pLibrary);
	if (nError != 0)
	{
		return 1;
	}
	FT_Face pFace = nullptr;
	nError = FT_New_Memory_Face(pLibrary, &*vFontFile.begin(), uFontFileSize, nFaceIndex, &pFace);
	if (nError != 0)
	{
		FT_Done_FreeType(pLibrary);
		return 1;
	}
	nError = FT_Set_Pixel_Sizes(pFace, 0, uPixelSize);
	if (nError != 0)
	{
		FT_Done_Face(pFace);
		FT_Done_FreeType(pLibrary);
		return 1;
	}
	map<Char16_t, SGlyph> mGlyph;
	n32 nTopMax = 0;
	n32 nBottomMin = 0;
	u32 uWidthMax = 0;
	for (vector<Char16_t>::iterator it = vCharset.begin(); it != vCharset.end(); ++it)
	{
		Char16_t uUnicode = *it;
		if (uUnicode == 0)
		{
			continue;
		}
		u32 uGlyphIndex = FT_Get_Char_Index(pFace, uUnicode);
		if (bMono)
		{
			nError = FT_Load_Glyph(pFace, uGlyphIndex, FT_LOAD_DEFAULT);
		}
		else
		{
			nError = FT_Load_Glyph(pFace, uGlyphIndex, FT_LOAD_NO_BITMAP);
		}
		if (nError != 0)
		{
			FT_Done_Face(pFace);
			FT_Done_FreeType(pLibrary);
			return 1;
		}
		FT_GlyphSlot pGlyph = pFace->glyph;
		if (bMono)
		{
			nError = FT_Render_Glyph(pGlyph, FT_RENDER_MODE_MONO);
		}
		else
		{
			if (fBold != 0.0)
			{
				FT_Outline_Embolden(&pGlyph->outline, static_cast<n32>(fBold * 64));
			}
			nError = FT_Render_Glyph(pGlyph, FT_RENDER_MODE_NORMAL);
		}
		if (nError != 0)
		{
			FT_Done_Face(pFace);
			FT_Done_FreeType(pLibrary);
			return 1;
		}
		FT_Bitmap ftBitmap = pGlyph->bitmap;
		vector<u8> vBuffer(ftBitmap.width * ftBitmap.rows);
		switch (ftBitmap.pixel_mode)
		{
		case FT_PIXEL_MODE_MONO:
			if (ftBitmap.num_grays != 2 && ftBitmap.num_grays != 0)
			{
				FT_Done_Face(pFace);
				FT_Done_FreeType(pLibrary);
				return 1;
			}
			for (u32 i = 0; i < ftBitmap.rows; i++)
			{
				for (u32 j = 0; j < ftBitmap.width; j++)
				{
					vBuffer[i * ftBitmap.width + j] = (ftBitmap.buffer[i * ftBitmap.pitch + j / 8] >> (7 - j % 8) & 1) * 0xFF;
				}
			}
			break;
		case FT_PIXEL_MODE_GRAY:
			if (ftBitmap.num_grays != 256 && ftBitmap.num_grays != 0)
			{
				FT_Done_Face(pFace);
				FT_Done_FreeType(pLibrary);
				return 1;
			}
			for (u32 i = 0; i < ftBitmap.rows; i++)
			{
				for (u32 j = 0; j < ftBitmap.width; j++)
				{
					vBuffer[i * ftBitmap.width + j] = ftBitmap.buffer[i * ftBitmap.pitch + j];
				}
			}
			break;
		default:
			FT_Done_Face(pFace);
			FT_Done_FreeType(pLibrary);
			return 1;
		}
		for (u32 i = 0; i < ftBitmap.rows; i++)
		{
			for (u32 j = 0; j < ftBitmap.width; j++)
			{
				vBuffer[i * ftBitmap.width + j] = vBuffer[i * ftBitmap.width + j] / 0x11 * 0x11;
			}
		}
		n32 nBitmapLeft = 0;
		n32 nBitmapRight = ftBitmap.width;
		n32 nBitmapTop = 0;
		n32 nBitmapBottom = ftBitmap.rows;
		for (u32 i = 0; i < ftBitmap.width; i++)
		{
			bool bEmpty = true;
			for (u32 j = 0; j < ftBitmap.rows; j++)
			{
				if (vBuffer[j * ftBitmap.width + i] != 0)
				{
					bEmpty = false;
					break;
				}
			}
			if (bEmpty)
			{
				nBitmapLeft++;
			}
			else
			{
				break;
			}
		}
		for (u32 i = 0; i < ftBitmap.width - nBitmapLeft; i++)
		{
			bool bEmpty = true;
			for (u32 j = 0; j < ftBitmap.rows; j++)
			{
				if (vBuffer[j * ftBitmap.width + ftBitmap.width - 1 - i] != 0)
				{
					bEmpty = false;
					break;
				}
			}
			if (bEmpty)
			{
				nBitmapRight--;
			}
			else
			{
				break;
			}
		}
		for (u32 i = 0; i < ftBitmap.rows; i++)
		{
			bool bEmpty = true;
			for (u32 j = 0; j < ftBitmap.width; j++)
			{
				if (vBuffer[i * ftBitmap.width + j] != 0)
				{
					bEmpty = false;
					break;
				}
			}
			if (bEmpty)
			{
				nBitmapTop++;
			}
			else
			{
				break;
			}
		}
		for (u32 i = 0; i < ftBitmap.rows - nBitmapTop; i++)
		{
			bool bEmpty = true;
			for (u32 j = 0; j < ftBitmap.width; j++)
			{
				if (vBuffer[(ftBitmap.rows - 1 - i) * ftBitmap.width + j] != 0)
				{
					bEmpty = false;
					break;
				}
			}
			if (bEmpty)
			{
				nBitmapBottom--;
			}
			else
			{
				break;
			}
		}
		u32 uWidthOld = ftBitmap.width;
		pGlyph->bitmap_left += nBitmapLeft;
		ftBitmap.width -= nBitmapLeft + (ftBitmap.width - nBitmapRight);
		pGlyph->bitmap_top -= nBitmapTop;
		ftBitmap.rows -= nBitmapTop + (ftBitmap.rows - nBitmapBottom);
		SGlyph& glyph = mGlyph[uUnicode];
		glyph.Height = ftBitmap.rows;
		glyph.Width = ftBitmap.width;
		glyph.Left = pGlyph->bitmap_left;
		glyph.Top = pGlyph->bitmap_top;
		glyph.Advance = pGlyph->metrics.horiAdvance / 64;
		vector<u8>& vData = glyph.Data;
		vData.resize(glyph.Height * glyph.Width);
		for (u32 i = 0; i < glyph.Height; i++)
		{
			for (u32 j = 0; j < glyph.Width; j++)
			{
				vData[i * glyph.Width + j] = vBuffer[(nBitmapTop + i) * uWidthOld + nBitmapLeft + j];
			}
		}
		if (glyph.Top > nTopMax)
		{
			nTopMax = glyph.Top;
		}
		if (glyph.Top - static_cast<n32>(glyph.Height) < nBottomMin)
		{
			nBottomMin = glyph.Top - static_cast<n32>(glyph.Height);
		}
		if (glyph.Width > uWidthMax)
		{
			uWidthMax = glyph.Width;
		}
		if (glyph.Advance > uWidthMax)
		{
			uWidthMax = glyph.Advance;
		}
	}
	FT_Done_Face(pFace);
	FT_Done_FreeType(pLibrary);
	u32 uHeightMax = nTopMax - nBottomMin;
	u32 uBlockWidth = uWidthMax + 4 + nPaddingLeft + nPaddingRight;
	u32 uBlockHeight = uHeightMax + 6 + nPaddingTop + nPaddingBottom;
	u32 uBlockColumn = 16;
	u32 uBlockRow = static_cast<u32>(Align(vCharset.size(), 16) / 16);
	u32 uBitmapFileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + uBlockColumn * uBlockWidth * uBlockRow * uBlockHeight * 4;
	vector<u8> vBitmapFile(uBitmapFileSize);
	BITMAPFILEHEADER* pBitmapFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(&*vBitmapFile.begin());
	pBitmapFileHeader->bfType = SDW_CONVERT_ENDIAN16('BM');
	pBitmapFileHeader->bfSize = uBitmapFileSize;
	pBitmapFileHeader->bfReserved1 = 0;
	pBitmapFileHeader->bfReserved2 = 0;
	pBitmapFileHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	BITMAPINFOHEADER* pBitmapInfoHeader = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeader + 1);
	pBitmapInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfoHeader->biWidth = uBlockColumn * uBlockWidth;
	pBitmapInfoHeader->biHeight = uBlockRow * uBlockHeight;
	pBitmapInfoHeader->biPlanes = 1;
	pBitmapInfoHeader->biBitCount = 32;
	pBitmapInfoHeader->biCompression = BI_RGB;
	pBitmapInfoHeader->biSizeImage = pBitmapInfoHeader->biWidth * pBitmapInfoHeader->biHeight * 4;
	pBitmapInfoHeader->biXPelsPerMeter = 0;
	pBitmapInfoHeader->biYPelsPerMeter = 0;
	pBitmapInfoHeader->biClrUsed = 0;
	pBitmapInfoHeader->biClrImportant = 0;
	u32* pBitmap = reinterpret_cast<u32*>(pBitmapInfoHeader + 1);
	for (u32 i = 0; i < vCharset.size(); i++)
	{
		u32 uBlockPosX = i % uBlockColumn;
		u32 uBlockPosY = i / uBlockColumn;
		for (u32 uY = 0; uY < uBlockHeight; uY++)
		{
			for (u32 uX = 0; uX < uBlockWidth; uX++)
			{
				u32 uColor = 0x00FFFFFF;
				if (uX == 0 || uX == uBlockWidth - 1 || uY == 0 || uY == uBlockHeight - 1)
				{
					uColor = 0x00000000;
				}
				else if (uX == 1 || uX == uBlockWidth - 2 || uY == 1 || uY == uBlockHeight - 4 || uY == uBlockHeight - 2)
				{
					uColor = 0x0099AA99;
				}
				pBitmap[(pBitmapInfoHeader->biHeight - 1 - uBlockPosY * uBlockHeight - uY) * pBitmapInfoHeader->biWidth + uBlockPosX * uBlockWidth + uX] = uColor;
			}
		}
		Char16_t uUnicode = vCharset[i];
		if (uUnicode == 0)
		{
			for (n32 nY = 0; nY < static_cast<n32>(uHeightMax) + nPaddingTop + nPaddingBottom; nY++)
			{
				for (n32 nX = 0; nX < static_cast<n32>(uWidthMax) + nPaddingLeft + nPaddingRight; nX++)
				{
					pBitmap[(pBitmapInfoHeader->biHeight - 1 - uBlockPosY * uBlockHeight - 2 - nY) * pBitmapInfoHeader->biWidth + uBlockPosX * uBlockWidth + 2 + nX] = 0x00FF7F7F;
				}
			}
			continue;
		}
		map<Char16_t, SGlyph>::iterator it = mGlyph.find(uUnicode);
		if (it == mGlyph.end())
		{
			return 1;
		}
		SGlyph& glyph = it->second;
		n32 nWidth = glyph.Width;
		nWidth += glyph.Left;
		if (nWidth < static_cast<n32>(glyph.Advance))
		{
			nWidth = glyph.Advance;
		}
		if (glyph.Left < 0)
		{
			nWidth -= glyph.Left;
		}
		n32 nX0 = (uWidthMax - nWidth) / 2;
		n32 nY0 = nTopMax - glyph.Top;
		n32 nLeft = glyph.Left < 0 ? 0 : glyph.Left;
		n32 nTestWidth = static_cast<n32>(uWidthMax) + nPaddingLeft + nPaddingRight;
		n32 nTestHeight = static_cast<n32>(uHeightMax) + nPaddingTop + nPaddingBottom;
		for (u32 uY = 0; uY < glyph.Height; uY++)
		{
			n32 nTestY = nY0 + static_cast<n32>(uY) + nPaddingTop;
			if (nTestY < 0 || nTestY >= nTestHeight)
			{
				continue;
			}
			for (u32 uX = 0; uX < glyph.Width; uX++)
			{
				n32 nTestX = nX0 + nLeft + static_cast<n32>(uX) + nPaddingLeft;
				if (nTestX < 0 || nTestX >= nTestWidth)
				{
					continue;
				}
				u32 uData = glyph.Data[uY * glyph.Width + uX];
				u32 uColor = (uData << 24) | ((0xFF - uData) << 16) | ((0xFF - uData) << 8) | (0xFF - uData);
				pBitmap[(pBitmapInfoHeader->biHeight - 1 - uBlockPosY * uBlockHeight - 2 - nPaddingTop - nY0 - uY) * pBitmapInfoHeader->biWidth + uBlockPosX * uBlockWidth + 2 + nPaddingLeft + nX0 + nLeft + uX] = uColor;
			}
		}
		nLeft = glyph.Left > 0 ? 0 : -glyph.Left;
		n32 nDelta = 0;
		if (fBold > 0)
		{
			nDelta = static_cast<n32>(ceil(static_cast<f64>(fBold / 2)));
		}
		for (n32 nX = -nDelta; nX < static_cast<n32>(glyph.Advance) + nDelta; nX++)
		{
			n32 nTestX = nX0 + nLeft + nX + nPaddingLeft;
			if (nTestX < 0 || nTestX >= nTestWidth)
			{
				continue;
			}
			pBitmap[(pBitmapInfoHeader->biHeight - 1 - uBlockPosY * uBlockHeight - (uBlockHeight - 3)) * pBitmapInfoHeader->biWidth + uBlockPosX * uBlockWidth + 2 + nPaddingLeft + nX0 + nLeft + nX] = 0x00FF0000;
		}
	}
	pBitmap[(pBitmapInfoHeader->biHeight - 1 - 2 - nTopMax - nPaddingTop - nBase) * pBitmapInfoHeader->biWidth] = 0x00FFFFFF;
	if (pBitmapInfoHeader->biHeight > 0)
	{
		pBitmap[(pBitmapInfoHeader->biHeight - 1) * pBitmapInfoHeader->biWidth] = 0x00FFFFFF;
	}
	fp = UFopen(argv[2], USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite(&*vBitmapFile.begin(), 1, uBitmapFileSize, fp);
	fclose(fp);
	return 0;
}
