/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id$
 */

#include "stdafx.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h> 

#include "ImageIL.h"
#include "Helpers.h"
#include "WinHelper.h"
#include "RegEx.h"

namespace Stamina {



	oImage loadImageFromFile(const char* filenameString, loadImageParams loadParams) {
		CStdString filename = Stamina::expandEnvironmentStrings( filenameString );
		const char * ext = strrchr(filename, '.');
		if (filename.length() < 5 || !ext)
			return oImage();
        ext++;
		if (!stricmp(ext, "ico")) {
			return Image::loadIcon32(filename, loadParams.size, loadParams.bits);
		} else if (!stricmp(ext, "bmp")) {
			return new Bitmap(filename, loadParams.transparent);
		}
		return new ImageIL(filename);
	}

	oImage loadImageFromResource(HINSTANCE inst, const char* resType, const char* resId, int imageType, loadImageParams loadParams) {
		if (imageType == IMAGE_ICON) 
			return Image::loadIcon32(inst, resId, loadParams.size, loadParams.bits);
//			return new Icon(inst, resId, loadParams.size, loadParams.bits);
		else if (imageType == IMAGE_BITMAP)
			return new Bitmap(inst, resId, loadParams.transparent);
		else {
			return new ImageIL(inst, resType, resId);
		}
	}

	oImage loadImageByUrl(const char* urlString, HINSTANCE dll, loadImageParams loadParams) {
        RegEx url;
		url.setSubject(urlString);
		url.match("|^([a-z]+)://(.+)\\.([^.#]*)(?:#(.+))?$|");
		if (!url.isMatched())
			return oImage();
		
		int imageType = (!stricmp(url[3].c_str(),"bmp"))?IMAGE_BITMAP:((!stricmp(url[3].c_str(),"ico"))?IMAGE_ICON:-1);
//		if (!stricmp(url[3].c_str(),"ico")) {
			CStdString params = "&" + url[4];
			CStdString::size_type start;
			start = params.find("&size=");
			if (!loadParams.size && start != -1) 
				loadParams.size = atoi(params.c_str() + start + strlen("&size="));
			start = params.find("&bits=");
			if (!loadParams.bits && start != -1) 
				loadParams.bits = atoi(params.c_str() + start + strlen("&bits="));
			if (params.find("&trans=1") != -1) 
				loadParams.transparent = true;
//		}

        if (url[1]=="file") {
			return loadImageFromFile((url[2]+"."+url[3]).c_str(), loadParams);
		} else if (url[1]=="res") {
			RegEx res;
            res.match("|^([0-9dlui]+)/(.+)$|" , url[2].c_str());
			HINSTANCE inst = (res[1]=="0" || res[1]=="ui")?Stamina::getInstance():res[1]=="dll"?dll:(HINSTANCE)atoi(res[1].c_str());
			// Wy�apujemy z RES nazw� typu i identyfikator
			res.match("|^(?:([^/]+)/)?([^/]+)$|" , res[2].c_str());
			CStdString type = res[1].c_str();
			CStdString id = res[2].c_str();
			res.setPattern("/^(?:\\d+)|(?:0x[0-9a-f]+)$/");
			res.setSubject(id);
			const char * idId = (res.match()) ? MAKEINTRESOURCE(chtoint(id)) : id;
			res.setSubject(type);
			const char * typeId = (res.match()) ? MAKEINTRESOURCE(chtoint(type)) : type;
			return loadImageFromResource(inst, typeId, idId, imageType, loadParams);
		} else if (url[1]=="handle") {
			if (imageType < 0) return 0;
			bool shared = (params.find("&shared=1") != -1);
			HANDLE imageHandle = (HANDLE)chtoint(url[2].c_str());
			if (!shared)
				imageHandle = CopyImage((HANDLE) imageHandle , imageType , 0 , 0 , 0);
			if (imageType == IMAGE_BITMAP) {
				return new Bitmap32((HBITMAP)imageHandle, false, shared);
			} else if (imageType == IMAGE_ICON) {
				return new Icon((HICON)imageHandle, shared);
			}
		} else {
			bool badUrlType = false;
			S_ASSERT(badUrlType);
		}
		return oImage();
	}



	ImageIL::ImageIL(const char* filename) {
	    ILuint imgID;
		ilInit();
		ilGenImages(1, &imgID);
		ilBindImage(imgID);
		ilEnable(IL_FORMAT_SET); 
		ilSetInteger(IL_FORMAT_MODE, IL_RGBA);
		ilLoadImage((ILstring)filename);
		this->setImage(imgID);
		ilDeleteImages(1, &imgID);
	}
	ImageIL::ImageIL(HINSTANCE inst, const char* resType, const char* resId) {
		int dataSize=0;
		HGLOBAL rsrc;
		void * data = loadResourceData(inst, resId, resType , rsrc, &dataSize);
		S_ASSERT(data);
		this->setImage(data, dataSize);
		FreeResource(rsrc);
	}

	void ImageIL::setImage(void* pixels, unsigned int dataSize) {
		S_ASSERT(pixels);
		ILuint imgID;
		ilInit();
		ilGenImages(1, &imgID);
		ilBindImage(imgID);
		ilEnable(IL_FORMAT_SET); 
		ilSetInteger(IL_FORMAT_MODE, IL_RGBA);
		int ilType = IL_TYPE_UNKNOWN;
		ilLoadL(ilType, pixels, dataSize);
		this->setImage(imgID);
		ilDeleteImages(1, &imgID);
	}

	void ImageIL::setImage(unsigned int ilid) {
		ILuint imgID = ilid;
		ilBindImage(imgID);
		int width = ilGetInteger(IL_IMAGE_WIDTH);
		int height = ilGetInteger(IL_IMAGE_HEIGHT);
		// pusta bitmapa jako maska...
		//ii.hbmMask = CreateBitmap(width,height,1,1,NULL);
		void * bmpData;
		BITMAPV5HEADER bi;
		ZeroMemory(&bi,sizeof(BITMAPV5HEADER));
		bi.bV5Size = sizeof(BITMAPV5HEADER);
		bi.bV5Width = width;
		bi.bV5Height = height;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		// The following mask specification specifies a supported 32 BPP
		// alpha format for Windows XP.
		bi.bV5RedMask   =  0x00FF0000;
		bi.bV5GreenMask =  0x0000FF00;
		bi.bV5BlueMask  =  0x000000FF;
		bi.bV5AlphaMask =  0xFF000000; 
		HDC hdc;
		hdc = GetDC(NULL);
		// Create the DIB section with an alpha channel.
		HBITMAP bmp = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, 			(void **)&bmpData, NULL, (DWORD)0);
		//memcpy(bmpData , ilGetData() , width * height * 4);
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
		ilCopyPixels(0,0,0,width,height,1,IL_BGRA,IL_UNSIGNED_BYTE,bmpData);
		ReleaseDC(0 , hdc);
		this->setBitmap(bmp, true, false);
	}


};
