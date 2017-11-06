#include <sdw.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_UPSAMPLE     STBIR_FILTER_BOX
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE   STBIR_FILTER_BOX
#include <stb_image_resize.h>

u32 getBmpRGB(u8* a_pBmp, u32 a_uX, u32 a_uY)
{
	BITMAPFILEHEADER* pBitmapFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(a_pBmp);
	BITMAPINFOHEADER* pBitmapInfoHeader = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeader + 1);
	u32 uOffset = pBitmapFileHeader->bfOffBits + ((pBitmapInfoHeader->biHeight - 1 - a_uY) * pBitmapInfoHeader->biWidth + a_uX) * (pBitmapInfoHeader->biBitCount / 8);
	u32 uRGB = *reinterpret_cast<u32*>(a_pBmp + uOffset);
	return uRGB;
}

void setBmpRGB(u8* a_pBmp, u32 a_uX, u32 a_uY, u32 a_uRGB)
{
	BITMAPFILEHEADER* pBitmapFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(a_pBmp);
	BITMAPINFOHEADER* pBitmapInfoHeader = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeader + 1);
	u32 uOffset = pBitmapFileHeader->bfOffBits + ((pBitmapInfoHeader->biHeight - 1 - a_uY) * pBitmapInfoHeader->biWidth + a_uX) * (pBitmapInfoHeader->biBitCount / 8);
	*reinterpret_cast<u32*>(a_pBmp + uOffset) = a_uRGB;
}

int UMain(int argc, UChar* argv[])
{
	if (argc < 5 || argc % 2 != 1)
	{
		return 1;
	}
	n32 nReferenceCount = (argc - 3) / 2;
	vector<map<Char16_t, u32>> vCharsetOld(nReferenceCount);
	vector<vector<u8>> vBmpOld(nReferenceCount);
	vector<u32> vBlockWidthOld(nReferenceCount);
	vector<u32> vBlockHeightOld(nReferenceCount);
	for (n32 i = 0; i < nReferenceCount; i++)
	{
		FILE* fp = UFopen(argv[i * 2 + 3], USTR("rb"), false);
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
		map<Char16_t, u32>& mCharsetOld = vCharsetOld[i];
		for (u32 j = 0; j < static_cast<u32>(sTxt.size()); j++)
		{
			Char16_t uUnicode = sTxt[j];
			if (uUnicode >= 0x20)
			{
				mCharsetOld.insert(make_pair(uUnicode, static_cast<u32>(mCharsetOld.size())));
			}
		}
		fp = UFopen(argv[i * 2 + 4], USTR("rb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		fseek(fp, 0, SEEK_END);
		u32 uBmpSizeOld = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		vBmpOld[i].resize(uBmpSizeOld);
		u8* pBmpOld = &*vBmpOld[i].begin();
		fread(pBmpOld, 1, uBmpSizeOld, fp);
		fclose(fp);
		BITMAPFILEHEADER* pBitmapFileHeaderOld = reinterpret_cast<BITMAPFILEHEADER*>(pBmpOld);
		BITMAPINFOHEADER* pBitmapInfoHeaderOld = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeaderOld + 1);
		vBlockWidthOld[i] = pBitmapInfoHeaderOld->biWidth / 16;
		vBlockHeightOld[i] = pBitmapInfoHeaderOld->biHeight / static_cast<u32>(Align(mCharsetOld.size(), 16) / 16);
	}
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
	vector<Char16_t> vCharsetNew;
	for (u32 i = 0; i < static_cast<u32>(sTxt.size()); i++)
	{
		Char16_t uUnicode = sTxt[i];
		if (uUnicode >= 0x20)
		{
			vCharsetNew.push_back(uUnicode);
		}
	}
	fp = UFopen(argv[2], USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uBmpSizeNew = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	vector<u8> vBmpNew(uBmpSizeNew);
	u8* pBmpNew = &*vBmpNew.begin();
	fread(pBmpNew, 1, uBmpSizeNew, fp);
	fclose(fp);
	BITMAPFILEHEADER* pBitmapFileHeaderNew = reinterpret_cast<BITMAPFILEHEADER*>(pBmpNew);
	BITMAPINFOHEADER* pBitmapInfoHeaderNew = reinterpret_cast<BITMAPINFOHEADER*>(pBitmapFileHeaderNew + 1);
	u32 uBlockWidthNew = pBitmapInfoHeaderNew->biWidth / 16;
	u32 uBlockHeightNew = pBitmapInfoHeaderNew->biHeight / static_cast<u32>(Align(vCharsetNew.size(), 16) / 16);
	for (u32 i = 0; i < static_cast<u32>(vCharsetNew.size()); i++)
	{
		Char16_t uUnicode = vCharsetNew[i];
		if (uUnicode < 0x4E00 || uUnicode > 0x9FA5)
		{
			for (n32 j = 0; j < nReferenceCount; j++)
			{
				map<Char16_t, u32>& mCharsetOld = vCharsetOld[j];
				map<Char16_t, u32>::iterator itOld = mCharsetOld.find(uUnicode);
				if (itOld != mCharsetOld.end())
				{
					u8* pBmpOld = &*vBmpOld[j].begin();
					u32 uBlockWidthOld = vBlockWidthOld[j];
					u32 uBlockHeightOld = vBlockHeightOld[j];
					u32 uPosXOld = itOld->second % 16;
					u32 uPosYOld = itOld->second / 16;
					u32 uPosXNew = i % 16;
					u32 uPosYNew = i / 16;
					vector<u32> vBlockOld((uBlockWidthOld - 4) * (uBlockHeightOld - 4));
					for (u32 uY = 0; uY < uBlockHeightOld - 4; uY++)
					{
						for (u32 uX = 0; uX < uBlockWidthOld - 4; uX++)
						{
							vBlockOld[uY * (uBlockWidthOld - 4) + uX] = getBmpRGB(pBmpOld, uPosXOld * uBlockWidthOld + 2 + uX, uPosYOld * uBlockHeightOld + 2 + uY);
						}
					}
					vector<u32> vBlockNew((uBlockWidthNew - 4) * (uBlockHeightNew - 4));
					if (uBlockWidthOld == uBlockWidthNew && uBlockHeightOld == uBlockHeightNew)
					{
						memcpy(&*vBlockNew.begin(), &*vBlockOld.begin(), vBlockNew.size() * 4);
					}
					else
					{
						stbir_resize_uint8(reinterpret_cast<u8*>(&*vBlockOld.begin()), uBlockWidthOld - 4, uBlockHeightOld - 6, 0, reinterpret_cast<u8*>(&*vBlockNew.begin()), uBlockWidthNew - 4, uBlockHeightNew - 6, 0, 4);
						stbir_resize_uint8(reinterpret_cast<u8*>(&*vBlockOld.begin() + (uBlockHeightOld - 5) * (uBlockWidthOld - 4)), uBlockWidthOld - 4, 1, 0, reinterpret_cast<u8*>(&*vBlockNew.begin() + (uBlockHeightNew - 5) * (uBlockWidthNew - 4)), uBlockWidthNew - 4, 1, 0, 4);
						for (u32 uX = 0; uX < uBlockWidthNew - 4; uX++)
						{
							vBlockNew[(uBlockHeightNew - 6) * (uBlockWidthNew - 4) + uX] = vBlockOld[(uBlockHeightOld - 6) * (uBlockWidthOld - 4)];
							if (vBlockNew[(uBlockHeightNew - 5) * (uBlockWidthNew - 4) + uX] != 0x00FFFFFF)
							{
								vBlockNew[(uBlockHeightNew - 5) * (uBlockWidthNew - 4) + uX] = 0x00FF0000;
							}
						}
					}
					for (u32 uY = 0; uY < uBlockHeightNew - 4; uY++)
					{
						for (u32 uX = 0; uX < uBlockWidthNew - 4; uX++)
						{
							setBmpRGB(pBmpNew, uPosXNew * uBlockWidthNew + 2 + uX, uPosYNew * uBlockHeightNew + 2 + uY, vBlockNew[uY * (uBlockWidthNew - 4) + uX]);
						}
					}
					break;
				}
			}
		}
	}
	fp = UFopen(argv[2], USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite(pBmpNew, 1, uBmpSizeNew, fp);
	fclose(fp);
	return 0;
}
