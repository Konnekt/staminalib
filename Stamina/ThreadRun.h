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

#include <process.h>
#include <boost\shared_ptr.hpp>
#include <string>
#include <list>

#include "Thread.h"
#include "ObjectImpl.h"
#include "Semaphore.h"


namespace Stamina {

	template <class F> unsigned int __stdcall  __threadRunProc(void* func) {
		F* f = (F*)func;
		(*f)();
		delete f;
		return 0;
	}

	typedef unsigned int ( __stdcall * fThreadProc)( void * );
	template <class F> inline HANDLE threadRunEx(F& func, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
		F* f = new F(func);
		unsigned int id;
		HANDLE handle = (HANDLE) _beginthreadex(attr, 0, (fThreadProc)(__threadRunProc<F>), (void*)f, suspended ? CREATE_SUSPENDED : 0 , &id);
		if (threadId)
			*threadId = id;
		if (name && *name) {
			Thread::setName(name, id);
		}
		return handle;
	}
	template <class F> inline void threadRun(F& func, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
		CloseHandle(threadRunEx(func, name, attr, suspended, threadId));
	}

	// --------------------------

	typedef SharedPtr<class ThreadRunner> oThreadRunner;
	typedef SharedPtr<class ThreadRunnerStore> oThreadRunnerStore;

	/**
	
	*/
	class ThreadRunner: public SharedObject<iSharedObject> {
	public:

		static uintptr_t defaultRunner (const char* name, void * sec, unsigned stack,	fThreadProc cb, void * args, unsigned flag, unsigned * addr) {
			unsigned int id;
			uintptr_t handle = (uintptr_t) _beginthreadex(sec, stack, cb, args, flag, &id);
			if (addr) {
				*addr = id;
			}
			if (name && *name) {
				Thread::setName(name, id);
			}
			return handle;
		}


//		typedef uintptr_t (*fBeginThread)(void *, unsigned,
//			unsigned (__stdcall *) (void *), void *, unsigned, unsigned *);
		typedef uintptr_t (*fBeginThread)(const char*, void *, unsigned,
			unsigned (__stdcall *) (void *), void *, unsigned, unsigned *);

		typedef unsigned int ( __stdcall * fThreadProc)( void * );

		ThreadRunner(fBeginThread func = defaultRunner) {
			this->_beginThread = func;
		}
		
		template <class F> inline HANDLE runEx(F& func, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
			typedef unsigned int ( __stdcall * fThreadProc)( void * );
			F* f = new F(func);
			return this->beginThread(name, attr, 0, (fThreadProc)(__threadRunProc<F>), (void*)f, suspended ? CREATE_SUSPENDED : 0, threadId);
		}
		template <class F> inline void run(F& func, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
			CloseHandle(runEx(func, name, attr, suspended, threadId));
		}

		inline HANDLE runEx(fThreadProc func, void* param, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
			return this->beginThread(name, attr, 0, func, param, suspended ? CREATE_SUSPENDED : 0, threadId);
		}
		inline void run(fThreadProc func, void* param, const char* name = 0, SECURITY_ATTRIBUTES* attr = 0, bool suspended=false, unsigned int* threadId = 0) {
			CloseHandle(runEx(func, param, name, attr, suspended, threadId));
		}

	protected:

		virtual HANDLE beginThread(const char* name, void * sec, unsigned stack,	fThreadProc cb, void * args, unsigned flag, unsigned * addr) {
			HANDLE handle = (HANDLE) _beginThread(name, sec, stack, cb, args, flag, addr);
			return handle;
		}

		fBeginThread _beginThread;
	};


	// -------------------------------------------

	class ThreadRunnerStore: public ThreadRunner {
	public:

		ThreadRunnerStore(fBeginThread func = defaultRunner):ThreadRunner(func) {
		}

		~ThreadRunnerStore() {
			_list.clear();
		}

		int waitForThreads(int timeout, int globalTimeout, bool terminateOnTimeout);

	protected:

		struct RunParams {
			fThreadProc func;
			void * args;
			ThreadRunnerStore* store;
			std::string name;
		};
		struct ThreadItem {
			bool operator == (const ThreadItem& b) const {
				return thread == b.thread;
			}

			Thread* thread;
			std::string startName;
        };

		typedef std::list<ThreadItem> ThreadList;

		ThreadList _list;

		static uintptr_t __stdcall threadStore(RunParams* rp);


		virtual HANDLE beginThread(const char* name, void * sec, unsigned stack,	fThreadProc cb, void * args, unsigned flag, unsigned * addr);

	};


};