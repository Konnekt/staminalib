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
#include <boost\crc.hpp>

#include "Helpers.h"

#include "ObjectImpl.h"
#include "Mutex.h"



namespace Stamina {


	//const unsigned int libInstance = random(0, 0xFFFFFFF);

	unsigned int ObjectClassInfo::getUID() {
		if (_uid) return _uid;
		else {
			boost::crc_32_type crc;
			for (const char * name = _name; *name; ++name) {
				crc.process_byte(*name);
			}
			return _uid = crc.checksum();
		}
	}


#ifdef STAMINA_DEBUG
	tDebugObjects* debugObjects = new tDebugObjects();
	Lock* debugObjectsCS = new SimpleMutex();

	class __initializer {
	public:
		__initializer() {
		}
		~__initializer() {
			if (debugObjects) {
				Locker locker (debugObjectsCS);
				// nie kasujemy debugObjectsCS, �eby nie rozwali� ew. czekaj�cych destruktor�w
				debugObjectsCS = 0;
				delete debugObjects;
				debugObjects = 0;
				
			}
		}
	} _initializer;

#endif


};