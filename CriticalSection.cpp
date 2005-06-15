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

#include <windows.h>

#include "CriticalSection.h"

namespace Stamina {

CriticalSection_w32::CriticalSection_w32(){
	InitializeCriticalSection(&cs);
}
CriticalSection_w32::~CriticalSection_w32(){
	DeleteCriticalSection(&cs);
}


CriticalSection_::CriticalSection_() {
	this->currentThread = 0;
	this->count = 0;
	this->event = CreateEvent(0, FALSE, TRUE, 0);
}
CriticalSection_::~CriticalSection_() {
	CloseHandle(event);
}



void CriticalSection_::lock(){
    DWORD current = GetCurrentThreadId();
    // czekamy a� lock si� zwolni...
    while (this->count && this->currentThread!=current) {
		if (WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0) // czekamy a� si� zwolni...
			break;
    }
    // ustawiamy siebie
    this->currentThread = current;
    return;
}

void CriticalSection_::lock(DWORD timeout , char action){
    DWORD current = GetCurrentThreadId();
    // czekamy a� lock si� zwolni...
    int test=0;
    while (this->count && this->currentThread && this->currentThread!=current) {
		if (WaitForSingleObject(event, 100) == WAIT_OBJECT_0)
			break;
        test+=100;
        if (test == timeout) {
            switch (action) {
                case 0: return;
                case CRITICAL_SECTION_TA_EXCEPT: RaiseException(CRITICAL_SECTION_TIMEOUT_EXCEPTION , 0 , 0 , 0); 
                case CRITICAL_SECTION_TA_EXCEPT2: RaiseException(STATUS_TIMEOUT , 0 , 0 , 0); 
                case CRITICAL_SECTION_TA_THROW: throw 0;
            }
        }
    }
    // ustawiamy siebie
    this->currentThread = current;
    return;
}


void CriticalSection_::lock(DWORD timeout , char * where){
    DWORD current = GetCurrentThreadId();
    // czekamy a� lock si� zwolni...
    int test=0;
    while (this->count && this->currentThread && this->currentThread!=current) {
		if (WaitForSingleObject(event, 100) == WAIT_OBJECT_0)
			break;
        test+=100;
        if (test == timeout) {
//            static ULONG_PTR tab[1];
//            tab[0] = (ULONG_PTR) where;
            RaiseException(CRITICAL_SECTION_TIMEOUT_EXCEPTION , 0 , where?1:0 , (ULONG_PTR *)&where); 
        }
    }
    // ustawiamy siebie
    this->currentThread = current;
    return;
}



void CriticalSection_::unlock(){
    DWORD current = GetCurrentThreadId();
    // Locka mo�e zdejmowa� tylko aktualny w�a�ciciel
    if (this->count == 0 || this->currentThread!=current) return;
	if ( (--this->count) == 0 ) {
        this->currentThread=0;
		SetEvent(this->event); // zwolniony!
    }
    return;
}

bool CriticalSection_::canAccess(){
    return this->count==0 || this->currentThread==0 || this->currentThread == GetCurrentThreadId();
}
void CriticalSection_::access(){
    while (!canAccess()) {
        SleepEx(1,false);
    }
}



// ------------------------------------------------------------------------

void CriticalSection_WM::lock(void) {
	DWORD current = GetCurrentThreadId();
    // czekamy a� lock si� zwolni...
    while (this->count && this->currentThread!=current) {
		if (_WMProcess) {
			if (MsgWaitForMultipleObjectsEx(1 , &event , INFINITE , QS_ALLINPUT | QS_ALLPOSTMESSAGE , MWMO_ALERTABLE | MWMO_INPUTAVAILABLE) != WAIT_OBJECT_0)
				_WMProcess();
		} else {
			if (WaitForSingleObjectEx(event, INFINITE, TRUE) == WAIT_OBJECT_0)
				break;
		}
    }
    // ustawiamy siebie
    this->currentThread = current;
    return;
}
void CriticalSection_WM_cond::lock() {
	DWORD current = GetCurrentThreadId();
	if (current == forThread)
		CriticalSection_WM::lock();
	else
		CriticalSection_::lock();
}

};