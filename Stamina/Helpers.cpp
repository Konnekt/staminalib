/*

The contents of this file are subject to the Mozilla Public License
Version 1.1 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License from
/LICENSE.HTML in this package or at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is "Stamina.lib" library code, released Feb 1, 2006.

The Initial Developer of the Original Code is "STAMINA" - Rafa� Lindemann.
Portions created by STAMINA are 
Copyright (C) 2003-2006 "STAMINA" - Rafa� Lindemann. All Rights Reserved.

Contributor(s): 

--

$Id$

*/

#include "stdafx.h"
#include <deque>
#include <direct.h>
#include <io.h>
#include <stdstring.h>
#include "Helpers.h"

#if (_MSC_VER >= 1400)
#include <errno.h>
#endif


using namespace std;

namespace Stamina {

	struct __initializer {
		__initializer() {
			srand((int)time(0));
		}
	} _initializer;


	unsigned int random(unsigned int min, unsigned int max) {
		static bool seeded = false;
		if (!seeded) {
			randomSeed();
			seeded = true;
		}
		unsigned int r = rand();
		unsigned int width = RAND_MAX;
		if (max - min > width) {
			width = 0x3FFFFFFF;
			r = r | (rand() << 15);
			if (max - min > width) {
				width = 0xFFFFFFFF;
				r = r | (rand() << 30);
			}
		}
		r = round(r * (double(max - min) / double(width)));
		return r + min;
	}


	const char * inttoch(int v , char * s , int radix , int max , bool upper) {
		_itoa(v , s , radix);
		if (!*s) strcpy(s , "0");
		if (max>0 && strlen(s)!=(unsigned)max) {
			int sz=strlen(s);
			if (sz>max) {
				for (int i=0;i<max;i++) {
					s[i]=s[sz-max+i];
				}
			} else {
				for (int i=max-1;i>=0;i--) {
					//      s[i]='E';
					if (i<max-sz) s[i]='0';
					else s[i]=s[i-max+sz];
				}
			}
			s[max]='\0';

		}
		if (radix>10 && upper) _strupr(s);
		return s;
	}

/*	const char * inttoch(int v , int radix , int max , bool upper) {
		return inttoch(v , TLS().buff , radix , max , upper);
	}*/

	std::string inttostr(int v , int radix , int max , bool upper) {
		string s;
		inttoch(v, stringBuffer(s, 16) , radix,max,upper);
		stringRelease(s);
		return s;
	}

/*
	template <typename NUMBER, typename CHAR> NUMBER strToNumber(const CHAR * str , unsigned char base) {
		if (!str) return 0;
		bool sign=false; // ujemna
		// sprawdzamy znak ujemno�ci, kt�ry zawsze jest PRZED ew. definicj� bazy...
		if (*str == '-') {sign = true; str++;}
		// Sprawdzamy base..
		if (base == 0xFF) {
			if (str[0] == '#' || (str[0]=='0' && str[1]=='x')) {
				str+=(str[0] == '#')?1 : 2;
				base = 16;
			} else base = 10;
		}
		const char * last = str;
		// zliczamy od ko�ca, kt�ry najpierw trzeba znale��...
		while ((*last>='0' && *last<='9') || (*last>='a' && *last<='f') || (*last>='A' && *last<='F')) last++;
		if (last == str) return 0;
		last--; // Cofamy do pierwszego znaku...
		NUMBER l = 0; // liczba wyjsciowa
		NUMBER b = 1;  // base do potegi 0
		while (last >= str) { // wczytuje znaki od konca
			l += chval(*last)*b; // dodaje do l wartosc znaku * podstawa podniesiona do potegi
			b *= base; // "podnosi" base o potege
			last --;
		}
		return sign? -l : l;
	}
*/
	inline int checkIntChar(const char*& str, unsigned char& base) {
		int sign = 1;
		if (*str == '-') {sign = -1; str++;}
		// Sprawdzamy base..
		if (base == 0xFF) {
			if (str[0] == '#' || (str[0]=='0' && str[1]=='x')) {
				str += (str[0] == '#') ? 1 : 2;
				base = 16;
			} else base = 10;
		}
		return sign;
	}
	inline int checkIntChar(const wchar_t*& str, unsigned char& base) {
		int sign = 1;
		if (*str == L'-') {sign = -1; str++;}
		// Sprawdzamy base..
		if (base == 0xFF) {
			if (str[0] == L'#' || (str[0] == L'0' && str[1] == L'x')) {
				str += (str[0] == L'#') ? 1 : 2;
				base = 16;
			} else base = 10;
		}
		return sign;
	}

	int chtoint(const StringRef& str , unsigned char base) {
		if (str.isWide()) {
			const wchar_t* s = str.w_str();
			int sign = checkIntChar(s, base);
			return sign * wcstoul(s, 0, base);
		} else {
			const char* s = str.a_str();
			int sign = checkIntChar(s, base);
			return sign * strtoul(s, 0, base);
		}
		//return strToNumber<unsigned int>(str, base);
	}
	__int64 chtoint64(const StringRef& str , unsigned char base) {
		if (str.isWide()) {
			const wchar_t* s = str.w_str();
			int sign = checkIntChar(s, base);
			return sign * _wcstoui64(s, 0, base);
		} else {
			const char* s = str.a_str();
			int sign = checkIntChar(s, base);
			return sign * _strtoui64(s, 0, base);
		}
	}


	char * str_tr(char * str , const char * chIn , const char * chOut) {
		if (!str || !chIn || !chOut || !*chIn || !*chOut || strlen(chIn)!=strlen(chOut)) return str;
		char * c = str;
		while (*c) {
			char * pos = strchr((char*)chIn , *c);
			if (pos) {
				*c = chOut[pos - chIn];
			}
			c++;
		}
		return str;
	}


	const char * searchArray(const char * find , const char ** ar  , size_t count , bool getValue) {
		size_t findLen = strlen(find);
		for (size_t i=0; i<count; i++)

			if (strlen(ar[i]) >= findLen && !_strnicmp(find , ar[i] , findLen)) {
				if (ar[i][findLen] != 0 && ar[i][findLen] != '=')
					continue;
				if (getValue) {
					char * value = strchr((char*)ar[i] , '=');
					if (!value) 
						return 0;
					return value + 1;
				} else return ar[i];
			}
			return 0;
	}

	const wchar_t * searchArray(const wchar_t * find , const wchar_t ** ar  , size_t count , bool getValue) {
		size_t findLen = wcslen(find);
		for (size_t i=0; i<count; i++)

			if (wcslen(ar[i]) >= findLen && !_wcsnicmp(find , ar[i] , findLen)) {
				if (ar[i][findLen] != 0 && ar[i][findLen] != L'=')
					continue;
				if (getValue) {
					wchar_t * value = wcschr((wchar_t*)ar[i] , L'=');
					if (!value) 
						return 0;
					return value + 1;
				} else return ar[i];
			}
			return 0;
	}

	const char * getArgV(const char * const * argList , int argCount , const char * find , bool getValue , const char * def) {
		const char * r = searchArray(find , (const char**)argList , argCount , getValue);
		return r ? r : def;
	}

	const wchar_t * getArgV(const wchar_t * const * argList , int argCount , const wchar_t * find , bool getValue , const wchar_t * def) {
		const wchar_t * r = searchArray(find , (const wchar_t**)argList , argCount , getValue);
		return r ? r : def;
	}

	String getArgV(const StringRef& find , bool getValue , const StringRef& def) {
		if (find.isWide() || def.isWide()) {
			return getArgV((const wchar_t**)__wargv+1 , __argc-1 , find.w_str() , getValue , def.w_str());
		} else {
			return getArgV((const char**)__argv+1 , __argc-1 , find.a_str() , getValue , def.a_str());
		}
	}

	#ifdef __BORLANDC__
	#define VSNPRINTF vsnprintf
	#else
	#define VSNPRINTF _vsnprintf
	#endif

	char * _vsaprintf(const char *format, va_list ap)
	{
			va_list temp_ap = ap;
			char *buf = NULL, *tmp;
			int size = 0, res;

			if ((size = VSNPRINTF(buf, 0, format, ap)) < 1) {
					size = 128;
					do {
							size *= 2;
							if ((tmp = (char*)realloc(buf, size))==0) {
									free(buf);
									return NULL;
							}
							buf = tmp;
							res = VSNPRINTF(buf, size, format, ap);
					} while (res == size - 1 || res==-1);
			} else {
					if ((buf = (char*)malloc(size + 1))==0)
							return NULL;
			}                                    

			ap = temp_ap;

			VSNPRINTF(buf, size + 1, format, ap);

			return buf;
	}

	char * _saprintf(const char *format, ...) {
		va_list ap;

		va_start(ap, format);
		char * ret = _vsaprintf(format, ap);
		va_end(ap);
			return ret;
	}




void split(const StringRef & txt, const StringRef & splitter, tStringVector & list, bool all) {
	unsigned int start = 0, end;
	list.clear();
	while (start < txt.length()) { // dop�ki jest co kopiowa�
		end = txt.find(splitter, start);
		if (all || start != end)
			list.push_back(txt.substr(start, (end == -1 ? end : end - start)) );
		if (end == -1)
			break;
		start = end + splitter.length();
	}
}
void splitCommand(const StringRef& txt , char splitter ,  tStringVector& list) {
    if (txt.empty()) return;
    char end;
	size_t pos = 0;
	do {
	    // Ustalamy jakim znakiem ma si� zakonczyc parametr
		if (splitter && txt.a_str()[pos]=='"') {pos++; end = '"';}
        else end = splitter;
	    size_t fnd = txt.find(end , pos);
	    list.push_back(txt.substr(pos , (fnd != -1)?fnd-pos:fnd));
	    pos = fnd;
	    if (pos != -1) {
		    pos += (end=='"')?2:1;
            if (pos>=txt.size()) break;
	    } else break;
	} while (1);
}


	char * stripSlashes(char * str) {
		char * buff = str;
		int offset = 0;
		while (*buff) {
			if (*buff == '\\') {
				char ch = buff[1]; // jaki jest nast�pny?
				switch (ch) {
					case 'r': ch = '\r'; break;
					case 'n': ch = '\n'; break;
					case 't': ch = '\t'; break;
				}
				*(buff - offset) = ch;
				offset++;
                buff++;				
			} else {
				if (offset)
					*(buff - offset) = *buff;
			}
			buff++;
		}
		*(buff - offset) = *buff;
		return str;
	}


	std::string urlEncode(const std::string& str , char special , char * noChange) {
		string res;
		string::const_iterator str_it;
		res.reserve(str.size()+30);
		char buff [32];
		for (str_it=str.begin(); str_it != str.end(); str_it++) {
			if ((!noChange || !strchr(noChange , *str_it))&&(*str_it<'0'||*str_it>'9')&&(*str_it<'a'||*str_it>'z')&&(*str_it<'A'||*str_it>'Z')) {
				res.append(1 , special);
				_itoa((unsigned char)*str_it , buff , 16);
				if (!buff[1]) {buff[2]=0;buff[1]=buff[0];buff[0]='0';}
				res.append(buff);
			} else {res.append(1, *str_it);}
		}
		return res;
	}

	std::string urlDecode(const std::string& in , char special) {
		std::string result = in;
		for (unsigned int i = 0; i<result.size(); i++) {
			if (result[i]==special) {
				result[i]=chtoint(result.substr(i+1,2).c_str() , 16);
				result.erase(i+1 , 2);
			}
		}
		return result;
	}

	std::string addSlashes(const std::string& str , char* escape, char escapeChar) {
		string res;
		string::const_iterator str_it;
		res.reserve(str.size()+30);
		for (str_it = str.begin(); str_it != str.end(); ++str_it) {
			if (strchr(escape, *str_it) != 0) {
				res.append(1 , escapeChar);
			}
			res.append(1, *str_it);
		}
		return res;
	}


	void * memrev(void * buff , int count) {
		char t;
		char * buf = (char*)buff;
		for (int i = 0 ; i < count / 2 ; i++) {
			t = buf[i];
			buf[i] = buf[count - i - 1];
			buf[count - i - 1] = t;
		}
		return buff;
	}

	std::string longToIp(long adr) {
		memrev(&adr , sizeof(adr));
		return stringf("%u.%u.%u.%u" , (adr&0xFF000000)>>24 , (adr&0xFF0000)>>16 , (adr&0xFF00)>>8 , adr&0xFF);
	}

	int ipToLong(const char * ip) {
		int a , b , c , d;
		sscanf(ip , "%u.%u.%u.%u" , &a , &b , &c , &d);
		return ((BYTE)d << 24) | ((BYTE)c<<16) | ((BYTE)b<<8) | (BYTE)a;
	}


// directories --------------------------------------------------

	int removeDirTree(const std::string& path) {
		if (path.empty()) return 0;
		WIN32_FIND_DATA fd;
		HANDLE hFile;
		BOOL found;
		found = ((hFile = FindFirstFile((path + "\\*.*").c_str(), &fd))!=INVALID_HANDLE_VALUE);
		//   int i = 0;
		int c = 1;
		while (found)
		{
			if (*fd.cFileName != '.') {
				std::string file = path + "\\";
				file += fd.cFileName;
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					c += removeDirTree(file);
				} else {
					_unlink(file.c_str());
					c++;
				}
			}
			if (!FindNextFile(hFile , &fd)) break; 
		}
		FindClose(hFile);
		_rmdir(path.c_str());
		return c;
	}


	std::string getCurrentDirectory() {
		std::string s;
		_getcwd(stringBuffer(s, 255), 255);
		stringRelease(s);
		return s;
	}


	int createDirectories(const std::string& path) {
		if (path.empty()) return 0;	
		int c = 0;
		if (_access(path.c_str() , 0) != 0) { // nie ma katalogu...
			size_t slash = path.find_last_of('\\');
			if (slash != -1) {
				c = createDirectories(path.substr(0, slash));
			}
			if (_mkdir(path.c_str())) {
				return c + 1;
			}
		}
		return c;
	}

	bool fileExists(const StringRef& file) {
		if (file.isWide()) {
			return _access(file.a_str(), 0) == 0;
		} else {
			return _waccess(file.w_str(), 0) == 0;
		}
	}

	bool isDirectory(const StringRef& path) {
		if (path == ".") return true;
		DWORD attr;
		if (path.isWide()) {
			attr = GetFileAttributesA(path.a_str());
		} else {
			attr = GetFileAttributesW(path.w_str());
		}
		if (attr == INVALID_FILE_ATTRIBUTES) 
			return false;
		else
			return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	std::string unifyPath(const std::string& path, bool slashSuffix, char delimiter) {
		if (path.empty()) return "";
		std::string res = path;

		for (std::string::iterator it = res.begin(); it != res.end(); ++it) {
			if (*it != delimiter && (*it == '/' || *it == '\\')) 
				*it = delimiter;
		}

		if (res[path.length() - 1] == delimiter) {
			if (slashSuffix == false) {
				res.erase(path.length() - 1);
			}
		} else {
			if (slashSuffix) {
				res += delimiter;
			}
		}
		return res;
		
	}


};