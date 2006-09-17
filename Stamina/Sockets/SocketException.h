/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2005-2006 Krzysztof G�ogocki
 *
 *  $Id:  $
 */

#ifndef __STAMINA_SOCKETEXCEPTION_H__
#define __STAMINA_SOCKETEXCEPTION_H__

#include <Stamina/Exception.h>
#include <Stamina/String.h>

namespace Stamina {
	class SocketException : public Exception {
	public:
		STAMINA_OBJECT_CLASS(Stamina::SocketException, Exception);

		SocketException(int errCode) {
			LPVOID lpMsgBuf;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf, 0, NULL))
			{
				_reason = (wchar_t*)lpMsgBuf;
                // Free the buffer.
                LocalFree(lpMsgBuf);
			}
		}

		SocketException(const StringRef& reason) : _reason(reason) {}

		virtual String getReason() const {
			return _reason;
		}

		inline bool hasReason() const {
			return !_reason.empty();
		}
		
		inline int getErrorCode() const {
			return _errCode;
		}

		virtual String toString(iStringFormatter *f = 0) const {
			return getReason();
		}
	private:
		String _reason;
		int _errCode;
	};
}

#endif	// __STAMINA_SOCKETEXCEPTION_H__