/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id: Lib.h 13 2005-06-17 18:02:57Z milka $
 */
#pragma once
#ifndef __STAMINA_LIBINSTANCE__
#define __STAMINA_LIBINSTANCE__


namespace Stamina { 

	class LibInstance {
	public:

		__declspec(dllexport) static LibInstance& get() {
			static LibInstance _instance;
			return _instance;
		}

	public:

		bool operator == (const LibInstance& b) {
			return this == &b; // por�wnujemy tylko wska�niki...
		}

		virtual void free(void* buff) {
			::free(buff);
		}

	private:
		LibInstance() {}
	};

}

#endif