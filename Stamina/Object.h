/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id$
 */

#pragma once

#ifndef __STAMINA_OBJECT__
#define __STAMINA_OBJECT__

#if defined(_DEBUG) && !defined(STAMINA_DEBUG)
#define STAMINA_DEBUG
#endif


#include "Assert.h"
#include "LibInstance.h"
#include "ObjectPtr.h"
#include "Version.h"
#include "Memory.h"

#ifdef STAMINA_DEBUG
	#include <list>
	#include <vector>
	#include "CriticalSection.h"
#endif

namespace Stamina {

#ifdef STAMINA_DEBUG
	typedef std::list<class iObject*> tDebugObjects;
	extern tDebugObjects* debugObjects;
	//extern CriticalSection* debugObjectsCS;
	extern Lock* debugObjectsCS;
	extern volatile long debugObjectsCount;
#endif


	class ObjectClassInfo {
	public:
		ObjectClassInfo(const char* name, short size, ObjectClassInfo* base, const Version& version = Version()):_name(name),_size(size),_base(base),_uid(0),_libInstance(LibInstance::get()), _version(version) {
		}
		inline const char * getName() const {
			return _name;
		}
		inline short getSize() const {
			return _size;
		}
		inline ObjectClassInfo* getBaseInfo() const {
			return _base;
		}
		unsigned int getUID();

		inline LibInstance& getLibInstance() const {
			return _libInstance;
		}

		inline const Version& getVersion() const {
			return _version;
		}

		inline ModuleVersion getModuleVersion() const {
			return ModuleVersion(versionClass, _name, _version);
		}

		inline bool operator == (ObjectClassInfo& b) {
			return this->getUID() == b.getUID();
		}
		/** Returns true if this object inherits from @a b */
		bool operator >= (ObjectClassInfo& b) {
			if (*this == b) {
				return true;
			} else if (this->getBaseInfo() == 0) {
				return false;
			} else {
				return *this->getBaseInfo() >= b;
			}
		}
		/** Returns true if this object is a base class for @a b */
		inline bool operator <= (ObjectClassInfo& b) {
			return b >= *this;
		}

		/** Looks for @a b class information */
		ObjectClassInfo* getParentInfo (ObjectClassInfo& b) {
			if (*this == b) {
				return this;
			} else if (this->getBaseInfo() == 0) {
				return 0;
			} else {
				return this->getBaseInfo()->getParentInfo(b);
			}
		}

		template <class TYPE> ObjectClassInfo* getParentInfo () {
			return getParentInfo(TYPE::staticClassInfo());
		}

	private:
		unsigned int _uid;
		const char * const _name;
		const short _size;
		ObjectClassInfo* const _base;
		LibInstance& _libInstance;
		const Version _version;
	};
#define STAMINA_OBJECT_CLASS_DEFINE(TYPE, NAME, BASE, VERSION) \
	typedef TYPE ObjectClass;\
	typedef BASE BaseClass;\
	static ::Stamina::ObjectClassInfo& staticClassInfo() {\
	static ::Stamina::ObjectClassInfo oci = ::Stamina::ObjectClassInfo(NAME, sizeof(TYPE), &BASE::staticClassInfo(), VERSION);\
		return oci;\
	}\
	::Stamina::ObjectClassInfo& getClass() const {\
		return staticClassInfo();\
	}

#define STAMINA_OBJECT_CLASS(TYPE, BASE) STAMINA_OBJECT_CLASS_DEFINE(TYPE, #TYPE, BASE, ::Stamina::Version())
	
#define STAMINA_OBJECT_CLASS_VERSION(TYPE, BASE, VERSION) STAMINA_OBJECT_CLASS_DEFINE(TYPE, #TYPE, BASE, VERSION)

#define STAMINA_OBJECT_CLONEABLE()\
	const static bool isCloneable = true;\
	virtual iObject* cloneObject() const {\
		iObject* obj = new ObjectClass();\
		obj->cloneMembers(this);\
		return obj;\
	}

	class String; // forward declaration
	class iStringFormatter;

	/** Basic object interface */
	class iObject {
	public:

#ifdef STAMINA_DEBUG

		iObject() {
			InterlockedIncrement(&debugObjectsCount);
/*			if (debugObjectsCS) {
				Locker locker(*debugObjectsCS);
				if (debugObjects) {
					debugObjects->push_back(this);
				}
			}
			*/
		}
		virtual ~iObject() {
			InterlockedDecrement(&debugObjectsCount);
			/*
			if (debugObjectsCS) {
				Locker locker (*debugObjectsCS);
				if (debugObjects) {
					debugObjects->remove(this);
				}
			}
			*/
		}
#else 
		iObject() {
		}

		virtual ~iObject() {};
#endif


		/** Returns object's class information */
		virtual ObjectClassInfo& getClass() const {
			return staticClassInfo();
		}

		virtual String toString(iStringFormatter* format=0) const;

		virtual iObject* cloneObject() const {
			S_DEBUG_ERROR("not cloneable!");
			return 0;
		}

	public:

		virtual void cloneMembers(const iObject* b) {
		}

	public:

		/** Static class information */
		static ObjectClassInfo& staticClassInfo() {
			static ObjectClassInfo oci = ObjectClassInfo("iObject", sizeof(iObject), 0, Version(0,1,0,0));
			return oci;
		}

		bool isFromCurrentLibInstance() {
			return this->getClass().getLibInstance() == LibInstance::get();
		}
		bool isSameLibInstance(const iObject& obj) {
			return this->getClass().getLibInstance() == obj.getClass().getLibInstance();
		}

		template <class TO> TO* castStaticObject() {
			return static_cast<TO*>(this);
		}

		template <class TO> TO* castStaticObject() const {
			return static_cast<TO*>(this);
		}

		template <class TO> TO* castObject() {
			if (this->getClass() >= TO::staticClassInfo()) {
				return reinterpret_cast<TO*>(this);
			} else {
				return 0;
			}
		}
		template <class TO> TO castObject(TO toClass) {
			if (this->getClass() >= toClass->getClass()) {
				return reinterpret_cast<TO>(this);
			} else {
				return 0;
			}
		}
		template <class TO> TO castObject(TO toClass) const {
			if (this->getClass() >= toClass->getClass()) {
				return reinterpret_cast<TO>(this);
			} else {
				return 0;
			}
		}

		void *operator new( size_t size) {
			return Memory::malloc(size);
		}

		void *operator new( size_t size, void* ptr) {
			return ptr;
		}

		void operator delete( void * buff ) {
			Memory::free(buff);
		}

		void operator delete( void * buff, void* ) {
			return;
		}


	private:

		virtual void zzPlaceHolder_iObject1() {}
		virtual void zzPlaceHolder_iObject2() {}

	};


	/** Interface of lockable objects */
	class iLockableObject: public iObject {
	public:
	    /** Blokuje dost�p do obiektu */
		virtual void lock() const {}
		/** Odblokowuje dost�p do obiektu */
		virtual void unlock() const {}

		virtual ~iLockableObject() {};

		STAMINA_OBJECT_CLASS_VERSION(Stamina::iLockableObject, iObject, Version(0,1,0,0));

	private:

		virtual void zzPlaceHolder_iLoObject1() {}
		virtual void zzPlaceHolder_iLoObject2() {}
		virtual void zzPlaceHolder_iLoObject3() {}
		virtual void zzPlaceHolder_iLoObject4() {}
		virtual void zzPlaceHolder_iLoObject5() {}

	};

	class iSharedObject: public iLockableObject {
	public:
		virtual bool hold() =0;
		virtual void release() =0;
		/** Returns true when it's safe to use the object */
		virtual bool isValid() =0;
		virtual bool isDestroyed() =0;
		virtual unsigned int getUseCount() =0;

		STAMINA_OBJECT_CLASS_VERSION(Stamina::iSharedObject, iLockableObject, Version(0,1,0,0));


#ifdef STAMINA_DEBUG

		iSharedObject() {
			if (debugObjectsCS) {
				Locker locker(*debugObjectsCS);
				if (debugObjects) {
					debugObjects->push_back(this);
				}
			}
		}
		virtual ~iSharedObject() {
			if (debugObjectsCS) {
				Locker locker (*debugObjectsCS);
				if (debugObjects) {
					debugObjects->remove(this);
				}
			}
		}
#else 
		iSharedObject() {
		}

		virtual ~iSharedObject() {};
#endif

	private:

		virtual void zzPlaceHolder_iShObject1() {}
		virtual void zzPlaceHolder_iShObject2() {}
		virtual void zzPlaceHolder_iShObject3() {}
		virtual void zzPlaceHolder_iShObject4() {}
		virtual void zzPlaceHolder_iShObject5() {}


	};

};


// For compatibility with boost::intrusive_ptr
namespace boost {
	inline void intrusive_ptr_add_ref(Stamina::iSharedObject* p) {
		p->hold();
	}
	inline void intrusive_ptr_release(Stamina::iSharedObject* p) {
		p->release();
	}
};

#include "String.h"



#endif