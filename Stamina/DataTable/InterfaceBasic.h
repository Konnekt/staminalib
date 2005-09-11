#pragma once
/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id: $
 */

#include "Interface.h"

namespace Stamina { namespace DT {

	class Interface_basic: public SharedObject<iInterface> {

	public:

		void showFileMessage(FileBase* file, const StringRef& _message, const StringRef& _title, bool error) {
			String msg;
			msg = error ? L"Wyst�pi� b��d w pliku " : L"Wyst�pi� problem z plikiem ";
			msg += "\"" + file->getFilename() + "\"\r\n\r\n";
			msg += _message; 

			String title = _title;
			title += " (" + getFileName(file->getFilename()) + ")";

			MessageBoxW(0, msg.w_str(), title.w_str(), MB_OK | (error ? MB_ICONERROR : MB_ICONWARNING));
		}


		bool handleMessageErrors(FileBase* file, DTException* e, const StringRef& title) {
			String msg;
			switch (e->errorCode) {
				case DT::errBadFormat: msg = L"Z�y format pliku"; break;
				case DT::errBadParameter: msg = L"Z�y parametr"; break;
				case DT::errBadType: msg = L"Z�y typ"; break;
				case DT::errBadVersion: msg = L"Nieobs�ugiwana wersja pliku - "; msg += file->getVersion().getString(); break;
				case DT::errFileError: msg = L"B��d systemu plik�w - "; msg += inttostr(e->errorCode & ~DT::errFileError); break;
				case DT::errFileNotFound: msg = L"Plik nie zosta� znaleziony"; break;
				case DT::errWriteError: msg = L"B��d zapisu"; break;
			}
			if (msg.empty() == false) {
				this->showFileMessage(file, msg, title, true);
			}
			return false;
		}

		virtual bool handleFailedLoad(FileBase* file, DTException* e, int retry) {
			if (e->errorCode == DT::errNotAuthenticated) {
				return this->handleNotAuthenticated(file, retry);
			} else if (file->getClass() >= FileBin::staticClassInfo()) {
				FileBin* fb = file->castObject<FileBin>();
				if (e->errorCode == DT::errBadFormat && fb->makeBackups) {
					return this->handleRestoreBackup(fb, e, retry);
				}
			}
			return handleMessageErrors(file, e, "B��d podczas wczytywania pliku");
		}

		virtual bool handleFailedSave(FileBase* file, DTException* e, int retry) {
			if (e->errorCode == DT::errNotAuthenticated) {
				return this->handleNotAuthenticated(file, retry);
			} else {
			}
			return handleMessageErrors(file, e, "B��d podczas zapisywania pliku");
		}

		virtual bool handleFailedAppend(FileBase* file, DTException* e, int retry) {
			if (e->errorCode == DT::errNotAuthenticated) {
				return this->handleNotAuthenticated(file, retry);
			} else if (file->getClass() >= FileBin::staticClassInfo()) {
				FileBin* fb = file->castObject<FileBin>();
				if (e->errorCode == DT::errBadFormat && fb->makeBackups) {
					return this->handleRestoreBackup(fb, e, retry);
				}
			}
			return handleMessageErrors(file, e, "B��d podczas dopisywania do pliku");
		}


		virtual bool handleNotAuthenticated(FileBase* file, int retry) {
			return false;
		}

		virtual bool handleRestoreBackup(FileBin* file, DTException* e, int retry) {
			return file->restoreLastBackup();
		}


	};


	class Interface_passList: public Interface_basic {
	public:

		typedef std::list<MD5Digest> tDigests;

	public:

		virtual bool handleNotAuthenticated(FileBase* file, int retry) {
			// pr�bujemy wszystkie z listy...
			MD5Digest oldDigest = file->getDT()->getPasswordDigest();
			for (tDigests::iterator it = _digests.begin(); it != _digests.end(); ++it) {
				file->getDT()->setPasswordDigest(*it);
				if (file->isAuthenticated()) {
					return true;
				}
			}
			// trzeba zapyta� usera o has�o...
			MD5Digest newDigest = this->askForPassword(file, retry);
			if (newDigest.empty() == false) {
				file->getDT()->setPasswordDigest(newDigest);
				if (file->isAuthenticated()) {
					// dzia�a! dopisujemy do listy... na pewno go na niej nie ma...
					_digests.push_back(newDigest);
					return true;
				}
			}
			// nic si� nie uda�o...
			file->getDT()->setPasswordDigest(oldDigest);
			return false;
		}

		virtual MD5Digest askForPassword(FileBase* file, int retry) {
			return MD5Digest();
		}

		void addPassword(const StringRef& pass) {
			MD5Digest digest = DataTable::createPasswordDigest(pass);
			if (std::find(_digests.begin(), _digests.end(), digest) == _digests.end()) {
				_digests.push_back(digest);
			}
		}

		tDigests& getDigests() {
			return _digests;
		}

	private:

		tDigests _digests;

	};


} } 