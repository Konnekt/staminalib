/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id$
 */

//#include <mem.h>
#include "stdafx.h"
#include "FileBin.h"


using namespace std;

namespace Stamina { namespace DT {

	FileBin::FileBin() { 
		init(); 
	}
	
	FileBin::FileBin(DataTable * t) {
		init();
		assign(t);
	}

	FileBin::~FileBin() {
		this->close();
	}

	void FileBin::init() {
		_passwordSalt = 0;
		_xorSalt = 0;

		_fcols.setLoader(true);
		_pos_data = _pos_rows = _pos_cols = _fileFlag = 0;
		_dataSize = 0;
		_dataFlag = basicDataFlags;
		useTempFile = false;
		//warn = true;
//		mode = 0;
    }

	void FileBin::reset() {
		FileBase::reset();
		_passwordSalt = 0;
		_xorSalt = 0;
		_passwordDigest.reset();
		_xorDigest.reset();
		_dataFlag = basicDataFlags;
	}

	/**Checks table's password digest with the file's one*/
	bool FileBin::isAuthenticated() {
		if (!_table) return false;
		MD5Digest digest(_table->getPasswordDigest());
		if (_passwordDigest.empty()) return true;
		if (_passwordSalt)
			digest.addSalt(_passwordSalt);
		return digest == _passwordDigest;
	}


	void FileBin::generatePasswordDigest(bool newSalt) {
		_passwordDigest = _table->getPasswordDigest();
		if (_passwordDigest.empty()) {
			_passwordDigest = MD5Digest("");
			_passwordSalt = 0;
		}
		if (newSalt || !_passwordSalt)
			_passwordSalt = random(1, 0x7FFFFFFF);
		_passwordDigest.addSalt(_passwordSalt);
	}

	void FileBin::generateXorDigest(bool newSalt) {
		_xorDigest = _table->getPasswordDigest();
		if (_xorDigest.empty()) {
			_xorDigest = MD5Digest("");
		}
		if (newSalt || !_xorSalt)
			_xorSalt = random(1, 0x7FFFFFFF);
		_xorDigest.addSalt(_xorSalt);
	}

	void FileBin::open (const std::string& fn , enFileMode mode) {
		this->close();

		this->setWriteFailed( false );
		_storedRowsCount=0;
		
		this->_opened = fileClosed;
		if (mode & fileRead) {
			_table->lastid = rowIdMin;
	        _table->dbID = -1;
		}

		fn = getFullPathName(fn);
		this->_fileName = fn;
	    
		if (useTempFile && (mode & fileWrite) && !(mode & fileRead)) {
			_temp_fileName = fn;
			int i = 0;
			do {
				_temp_fileName = fn + stringf(".[%d].tmp", ++i);
			} while ( ! _access(_temp_fileName , 0));
		} else {
			_temp_fileName = "";
		}

		bool fileExists = (_access(this->getOpenedFileName().c_str(),0) == 0);

		const char* openmode;
		// Dla Append, oraz Read+Write otwieramy w trybie read+write i je�li jest taka potrzeba, tworzymy plik na nowo
		if (((mode & fileAppend) || (mode & fileWrite && mode & fileRead))) {
			openmode = fileExists ? "w+b" : "r+b";
			_recreating = (fileExists == false);
			_table->_timeModified.now();
		} else if (mode & fileWrite) {
			openmode = "wb";
			_recreating = true;
			_table->_timeModified.now();
		} else {
			openmode = "rb";
			_recreating = false;
		}

		if (_table->_timeCreated.empty()) 
			_table->_timeCreated.now();

		_file=fopen(this->getOpenedFileName().c_str(), openmode);

		if (_file && useTempFile && (mode & fileWrite)) {
			_temp_enabled = true;
		} else {
			_temp_enabled = false;
			_temp_fileName = "";
		}

		if (!_file) throw DTFileException();

		this->updateFileSize();

		this->_opened = mode;

		// Zawsze wczytujemy nag��wek, chyba �e b�dziemy tworzy� plik od nowa.
		if (isReadable()) {
			this->readHeader();
		} else {
			// W przeciwnym razie uznajemy �e to co posiadamy jest "za�adowanym" nag��wkiem
			this->_headerLoaded = true;
		}

    }

    void FileBin::close () {
		if (!this->isOpened()) return;

		this->writeCount();
        fclose(_file);
		
		if (this->isWriteFailed()) {
		    _opened = fileClosed;
#ifdef _WINDOWS_
			std::string msg = string("Wyst�pi� b��d podczas zapisywania danych!\r\nUszkodzona kopia znajduje si� w:\r\n\t%s%s" , _temp_enabled ? _temp_fileName.c_str() : _filename.c_str(), _temp_enabled ? "\r\n\r\nOryginalny plik pozosta� bez zmian..." : "\r\n\r\nB��d wyst�pi� podczas zapisywania oryginalnej kopii!");
			int r = MessageBox(0 , msg.c_str() , "B��d zapisu .DTB" , MB_OK | MB_ICONERROR | MB_TASKMODAL);
#endif
			return;
		}

        if (_temp_enabled) { // trzeba przerzuci� tempa
            bool success = true;
			int retries = 0;
            while (1) {
				_unlink(_fileName);
				if (rename(_temp_fileName , _fileName) == 0)
					break; // uda�o si�!

				if (++retries < 8) { // czekamy 2 sekundy
					Sleep(250);
					continue;
				}
#ifdef _WINDOWS_
				std::string msg = stringf("Nie mog� zapisa� danych do pliku\r\n   %s" , _fileName.c_str());
				int r = MessageBox(0 , msg.c_str() , "B��d zapisu .DTB" , MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON2 | MB_TASKMODAL);
                if (r == IDIGNORE) {
					success=true;
					break;
				} else if (r == IDABORT) {
	                success = false;
					break;
				}
#endif
            }
            if (!success) _unlink(_temp_fileName);
		}

		_opened = fileClosed;
    }


// HEADER ** READ -----------------------------------------------------------

	void FileBin::readHeader() {
		if (!isOpened()) throw DTException(errNotOpened);
		if (this->_headerLoaded == true) return;

		this->seekToBeginning();
      
		char sig [6];
		sig[5]='\0';
		readData(sig , 5);
		if (strcmp("DTBIN" , sig)) throw DTException(errBadFormat);

		readData(&_verMaj , 1);
		if (_verMaj > versionMajor) throw DTException(errBadVersion);

		// �adujemy drugi cz�on wersji, oraz flagi (dost�pne od v2.0)
	    if (_verMaj > '1') {
			readData(&_verMin , 1);
			readData(&_fileFlag , 4);
		} else {
			_verMin = 0; 
			flag = 0;
		}

		_pos_count = ftell(_file);
		// �adujemy liczb� wierszy
		readData(&_storedRowsCount, 4);  // rowc
		_table->_size = _storedRowsCount;
      
		int csize , a , b;
		_table->_lastId = DT_ROWID_MIN;
		md5digest[0]=0;
		table->dbID = -1;

		_dataFlag = dflagNone;
        int dataLeft;
		// Pole DATA (od v2.0)
		if (_verMaj > '1') {
			readData(&_dataSize , 4);
	        _pos_data = ftell(_file);
			dataLeft = _dataSize;
		} else {
			_dataSize = 0;
		}
        if (_dataSize) {
			readData(&_dataFlag , 4, &dataLeft);
		}

        // �adujemy wszystkie wyszczeg�lnione pola
		if (_dataFlag & dflagLastId) {
			_pos_dataLastId = ftell(_file);
			readData(&_table->_lastId , 4, &dataLeft);
		} else {
			_pos_dataLastId = 0;
			_table->_lastId = rowIdMin;
		}
        
		if (_dataFlag & dflagPasswordDigest) {
			MD5Digest::tDigest digest;
			readData(digest , 16, &dataLeft);
			this->_passwordDigest.setDigest(digest);
		} else {
			this->_passwordDigest.reset();
		}

/*		if (_dataFlag & dflagDBId) {
			readData(&_table->_dbID , 4, &dataLeft);
		} else {
			_table->_dbID = -1;
		}*/
		if (_dataFlag & dflagDBId) { // nie obs�ugujemy ju� tego!
			this->setFilePosition(4, fromCurrent);
			dataLeft -= 4;
		}


		if (_dataFlag & dflagParams) {
			int paramCount;
			readData(&paramCount, 4, &dataLeft);
			for (int i = 0; i < paramCount; i++) {
				std::string key = readString(&dataLeft);
				std::string value = readString(&dataLeft);
				if (_table->paramExists(key) == false) {
					_table->setParam(key, value);
				}
			}
		}

		if (_dataFlag & dflagCreated) {
			readData(&_table->_timeCreated, 8, &dataLeft);
		} else {
			_table->_timeCreated.now();
		}

		if (_dataFlag & dflagModified) {
			readData(&_table->_timeModified, 8, &dataLeft);
		} else {
			_table->_timeModified.now();
		}

		if (_dataFlag & dflagLastBackup) {
			readData(&_table->_timeLastBackup, 8, &dataLeft);
		} else {
			_table->_timeCreated = 0;
		}

		if (_dataFlag & dflagPassSalt) {
			readData(&_passwordSalt, 4, &dataLeft);
		} else {
			_passwordSalt = 0;
		}

		if (_dataFlag & dflagXorSalt) {
			readData(&_xorSalt, 4, &dataLeft);
		} else {
			_xorSalt = 0;
		}


		// pomijamy ca�� reszt� kt�rej nie rozumiemy...
		if (dataLeft > 0) {
			this->setFilePosition(dataLeft, fromCurrent);
		}

		_pos_cols = ftell(_file);

		this->_headerLoaded = true;
		this->authenticate();

		// je�eli uda�o nam si� zautentykowa�, znaczy �e has�o jest prawid�owe i mo�emy spokojnie tworzy� mask� xor
		this->generateXorDigest(false);
	}

// HEADER ** WRITE ----------------------------------------------------------

	void FileBin::writeHeader() {
		if (!isOpened()) throw DTException(errNotOpened);

		try {

			// generujemy nowe has�o i mask� xor z nowymi saltami
			this->generatePasswordDigest(true);
			this->generateXorDigest(true);

			_verMaj = versionMajor;
			_verMin = versionMinor;

			writeData("DTBIN", 5); 

			writeData(&_verMaj, 1);
			writeData(&_verMin, 1);

			writeData(&_fileFlag, 4); // flag

			_pos_count = ftell(_file);
			writeData(&(_table->getRowCount()), 4);    // rowc

			_dataSize = 0x7FFFFFFF;
			// placeholder do zapisania troch� p�niej. Specjalnie zapisujemy du�� warto��, �eby w razie pozostawienia tego w takim stanie w pliku spowodowa� "bezpieczny" b��d podczas wczytywania
			writeData(&_dataSize, 4);

			_pos_data = ftell(_file);

			// usuwamy nie obs�ugiwane flagi
			_dataFlag = (enDataFlags)(_dataFlag & ~(deprecatedDataFlags));
			// dodajemy wymagane obs�ugiwane flagi
			_dataFlag = (enDataFlags)(_dataFlag | requiredDataFlags);

			if (! _table->getParamsMap().empty()) {
				_dataFlag = (enDataFlags)(_dataFlag | dflagParams);
			}

			if (_dataFlag) {
				writeData(&_dataFlag, 4, &_dataSize);
			}

			if (_dataFlag & dflagLastId) {
				_pos_dataLastId = ftell(_file);
				writeData(&_table->lastid, 4, &_dataSize);
			} else {
				_pos_dataLastId = 0;
			}

			if (_dataFlag & dflagPasswordDigest) {
				writeData(_passwordDigest.getDigest(), 16, &_dataSize);
			}

			if (_dataFlag & dflagParams) {
				writeData(&(_table->getParamsMap().size()), 4, &_dataSize);
				for (DataTable::tParams::iterator it = _table->getParamsMap().begin(); it != _table->getParamsMap().end(); it++) {
					writeString(it->first, &_dataSize);				
					writeString(it->second, &_dataSize);				
				}
			}
			if (_dataFlag & dflagCreated) {
				writeData(&_table->_timeCreated, 8, &_dataSize);
			}

			if (_dataFlag & dflagModified) {
				writeData(&_table->_timeModified, 8, &_dataSize);
			}

			if (_dataFlag & dflagLastBackup) {
				writeData(&_table->_timeLastBackup, 8, &_dataSize);
			}

			if (_dataFlag & dflagPassSalt) {
				writeData(&_passwordSalt, 4, &_dataSize);
			}

			if (_dataFlag & dflagXorSalt) {
				writeData(&_xorSalt, 4, &_dataSize);
			}

			// Zapisujemy w�a�ciwy rozmiar...
			this->setFilePosition(_pos_data - 4, fromBeginning);
			writeData(&_dataSize, 4);
			// Ustawiamy si� z powrotem za danymi
			this->setFilePosotion(_dataSize, fromCurrent);

			this->updateFileSize();

		} catch (DTException e) {
			this->setWriteFailed(true);
			throw e;
		}
	}

// DESCRIPTOR ** READ -------------------------------------------------------

	void FileBin::readDescriptor() {
		if (!isOpened()) throw DTException(errNotOpened);

		this->setFilePosition(_pos_cols, fromBeginning);

		int count;
		readData(&count, 4);
		_fcols.setcolcount(count);
		for (int i = 0; i < count; i++) { // columns definitions
			int id;
			int type;
			readData(&id, 4);
			readData(&type, 4);
			std::string name;
			/*Wczytujemy nazw� kolumny i dane dodatkowe (od v3.0)*/
			if (_verMaj>'2') {
				char length;
				readData(&length, 1);
				if (length) {
					readData(stringBuffer(name, length), length);
					stringRelease(name, length);
				}
				int dataSize;
				readData(&dataSize , 4);
				if (dataSize) {
					setFilePosition(dataSize, fromCurrent); // We have to get past unprocessed data.
				}
			}
			_fcols.setColumn(id, type, 0, name.c_str());
		}
		_pos_rows = ftell(_file);
	}

// DESCRIPTOR ** WRITE-------------------------------------------------------

	void FileBin::writeDescriptor() {
		if (!isOpened()) throw DTException(errNotOpened);

		try {

			writeData(&(_fcols.getColCount()), 4);

			for (i = 0; i < _fcols.getColCount(); i++) {
				const Column& col = _fcols.getColumnByIndex(i);
				writeData(&col.id, 4);   //id
				int type = col.getFlags() & (~DT_CF_NOSAVEFLAGS); 
				writeData(&type, 4);   //type
				unsigned char nameLength = min(255, col.name.size());
				writeData(&nameLength , 1); // name length
				if (nameLength) 
					writeData(col.name.c_str(), nameLength); // name
				dataSize = 0;
				writeData(&dataSize, 4); // For future use maybe.
			}
	      
			_pos_rows = ftell(_file);

			this->updateFileSize();

		} catch (DTException e) {
			this->setWriteFailed(true);
			throw e;
		}
	}

// SIZE ** READ -------------------------------------------------------------

	void FileBin::readCount() {
		if (!isOpened()) throw DTException(errNotOpened);
		// Ustawiamy si� na pozycj� zawieraj�c� rozmiar
		setFilePosition(_pos_count, fromBeginning);
		readData(&_storedRowsCount, 4);
    }

// SIZE ** WRITE ------------------------------------------------------------

	void FileBin::writeCount() {
		if (!isOpened()) throw DTException(errNotOpened);
		try {
			if ((getFileMode() & (fileWrite | fileAppend)) && _storedRowsCount != -1) {
                setFilePosition(_pos_count , fromBeginning);
				writeData(&_storedRowsCount, 4);
				// zapisujemy lastId (od v2.0)
				if (_table && _verMaj > '1' && _pos_dataLastId > 0) {
					setFilePosition(_pos_dataLastId, fromBeginning);
					writeData(&_table->_lastId, 4);
				}
			}
		} catch (DTException e) {
			this->setWriteFailed(true);
			throw e;
		}
    }

// ROW ** READ --------------------------------------------------------------

	enResult FileBin::readPartialRow(tRowId row , tColId* columns) {
		if (!isOpened()) throw DTException(errNotOpened);

		// Pokojowe wyj�cie - nie ma co czyta� wi�c ko�czymy
		if ( feof(_file) ) return resNothingToRead;
		// Od tego momentu wszystko czego nie da si� wczyta� oznacza z�y format!

		row = _table->getRowPos(row);
	  
		//_table->notypecheck=1;  // wylacza sprawdzanie typow ...
      
		DataRow& rowObj = _table->getRow(row);
		
		// zapiujemy pozycj� wiersza
		rowObj._filePos = ftell(_file);

		// Znacznik rozpocz�cia nowego wiersza
		if (fgetc(_file) != '\n') throw DTException(errBadFormat);

		unsigned int rowSize;
		unsigned int rowDataSize;
		enRowDataFlags rowDataFlag;
		unsigned int rowDataLeft;

		// rozmiar wiersza oraz flagi od v2.0
		if (_verMaj > '1') {
			readData(&rowSize, 4);
			// Sprawdzamy czy ten row jest w stanie si� tu w og�le zmie�ci�
			if (ftell(file) + rowSize + 4 > _fileSize)
				throw DTException(errBadFormat);

			//TODO: mo�e to wy��czy�??
			// Skaczemy do przodu �eby sprawdzi� czy rozmiary si� zgadzaj�
			setFilePosition(rowSize , fromCurrent);
			unsigned int rowSize2;
			readData(&rowSize2 , 4);
			setFilePosition(-rowSize - 4, fromCurrent); // Wracamy do pozycji
			if (rowSize != rowSize2) 
				throw DTException(errBadFormat);
            
			readData(&rowObj._flag, 4);

			// Sprawdzamy czy flaga jest r�wna -1 czyli czy element
			// nie jest oznaczony jako usuni�ty
			if (rowObj._flag == -1) {
				setFilePosition(rowSize, fromCurrent);
				// wywo�ywanie rekurencyjne jest potencjalnie niebezpieczne...
				return resSkipped;
			}

			readData(&rowDataSize, 4);
			if (rowDataSize > rowSize - 8)
				throw DTException(errBadFormat);

			rowDataLeft = rowDataSize;

			if (rowDataSize) {
				readData(&rowDataFlag, 4, &rowDataLeft);
			} else {
				rowDataFlag = rdflagNone;
			}
			
		} else { // v >= 2.0
			rowDataFlag = rdflagNone;
			rowDataSize = 0;
			rowDataLeft = 0;
		}


		if (rowDataFlag & rdflagRowId) {
			tRowId id;
			readData(&id, 4, &rowDataLeft);
			if (id != rowObj.getId()) {
				if (_table->rowIdExists(id)) {
					id = _table->getNewRowId();
				}
				rowObj.setId(id);
			}
		} else {
			// w zasadzie nie ma potrzeby przydziela� nowego ID, bo jest ju� przydzielony przy okazji utworzenia wiersza... Poza tym ka�dy szanuj�cy si� DTB zawiera t� warto��...
			// TODO: sprawdzi�!
		}

		// Pomijamy dane kt�rych nie rozumiemy...
		if (rowDataLeft > 0) setFilePosition(rowDataLeft, fromCurrent);

		// �adujemy dane
		for (unsigned int colIndex = 0; colIndex < _fcols.getColCount(); colIndex++) {
			const Column& col = _fcols.getColumnByIndex(colIndex);

			// skoro kolumna nie s�u�y do zapisywania - nie mamy co wczytywa�...
			if (col.hasFlag(cflagDontSave)) continue;

			// identyfikator kolumny w TABLICY
			tColId colId = col.getId();

			if (col.isIdUnique()) { 
				colId = _table->getColumns().getNameId(col.getName().c_str()); 
			}

			bool skip = (colId == colNotFound); // Czy OMIN�� dane kolumny?

			// szukamy, czy ID aktualnej, jest na li�cie, jak nie to omijamy...
			if (columns) { 
				skip = true;
				int i = 0;
				do {
					if (colId == columns[i]) {
						skip = false;
						break;
					}
					i++;
				} while (columns[i]);
			}

			unsigned int skipBytes = 0;

			switch (col.getType()) {
				case ctypeInt:
					if (skip)
						skipBytes = 4;
					else {
						int val;
						readCryptedData(col, &val, 4);
						rowObj.setByIndex(colIndex, (DataEntry)val);
					}
					break;
				case ctypeInt64:
					if (skip) {
						skipBytes = 8;
					} else {
						__int64 val;
						readCryptedData(col, &val, 8);
						rowObj.setByIndex(colIndex, (DataEntry)&val);
					}
					break;
				case ctypeString: {
					unsigned int length;
					readData(&length, 4);
					if (ftell(_file) + length > _fileSize)
						throw DTException(errBadFormat);
					if (skip) {
						skipBytes = length;
					} else if (length > 0) {
						char buffer = new char [length + 1];
						buffer[length] = 0;
						readCryptedData(col, buffer, length);
						rowObj.setByIndex(colIndex, (DataEntry)buffer);
						delete [] buffer;
					} else {
						rowObj.setByIndex(colIndex, (DataEntry)"");
					}
					break;}
				case ctypeBin: {
					TypeBin bin;
					bin.buff = 0;
					readData(&bin.size, 4); // wczytujemy rozmiar
					if (ftell(_file) + bin.size > _fileSize)
						throw DTException(errBadFormat);
					if (skip) {
						skipBytes = bin.size;
					} else if (bin.size > 0) {
						bin.buff = new char [bin.size];
						readCryptedData(col, bin.buff, bin.size);
						rowObj.setByIndex(colIndex, (DataEntry)&bin);
						delete [] bin.buff;
					} else {
						rowObj.setByIndex(colIndex, (DataEntry)&bin);
					}
					break; }

			}
			if (fgetc(file) != '\t')
				throw DTException(errBadFormat);
		} // kolumny

		// drugi rozmiar od v2.0
		if (_verMaj > '1') {
			unsigned int rowSize2;
			readData(&rowSize2, 4);
			if (rowSize != rowSize2) 
				throw DTException(errBadFormat);
		}
		return success;
    }


// ROW ** WRITE -------------------------------------------------------------

	void FileBin::writeRow(tRowId row) {
		if (!isOpened()) throw DTException(errNotOpened);

		row = _table->getRowPos(row);

		if (fputc('\n' , _file) == EOF)
			throw DTFileException();

		DataRow& rowObj = _table->getRow(row);

		unsigned int rowSize = 0;

		// rozmiar, flagi - od v2.0
		if (_verMaj > '1') {
			writeData(&rowSize, 4);  //rowSize - placeholder
			writeData(&(rowObj.getFlags()), 4, &rowSize); // flag
			unsigned int dataSize = 8; // flag + lastId, na razie nie ma wi�cej
			writeData(&dataSize, 4, &rowSize);
			enRowDataFlags flags = rdflagRowId;
			writeData(&flags, 4, &rowSize);
			writeData(&(rowObj.getId()), 4, &rowSize);
		}
		for (unsigned int colIndex =0; colIndex < fcols.cols.size(); colIndex++) {
			const Column& col = _fcols.getColumnByIndex(colIndex);

			if (col.hasFlag(rflagDontSave)) continue;

			tColId id = col.getId();
			if (col.isIdUnique()) { 
				id = _table->getColumns().getNameId(col.getName().c_str()); 
			}

			switch (col.getType()) {
				case ctypeInt: {
					int val = (int)rowObj.getByIndex(colIndex);
					writeCryptedData(col, &val, 4, &rowSize);
					break;}
				case ctype64: {
					__int64* val = (__int64*)rowObj.getByIndex(colIndex);
					if (!val) {
						// zapisujemy 0
						__int64 null = 0;
						writeCryptedData(col, &null, 8, &rowSize);
					} else {
						writeCryptedData(col, val, 8, &rowSize);
					}
					break;}
				case ctypeString: {
					char * val = (char *)rowObj.getByIndex(colIndex);
					unsigned int length = (val == 0 ? 0 : strlen(val));
					writeData(&length, 4, &rowSize);
					if (val && length > 0) {
						writeCryptedData(col, val, length, &rowSize);
					}
					break;}
				case ctypeBin: {
					TypeBin* val = (TypeBin*)rowObj.getByIndex(colIndex);
					writeData(&val->size, 4, &rowSize);
					if (val->buff && val->size > 0) {
						writeCryptedData(col, val->buff, val->size, &rowSize);
					}
					break;}
			}

			if (fputc('\t' , _file) == EOF)
				throw DTFileException();
			rowSize++;
		}
		// zapisujemy wynik od v2.0
		if (_verMaj > '1') {
			writeData(&rowSize, 4); // size
			// cofamy si� o ca�y wiersz i oba zapisane size'y
			setFilePosition(- rowSize - 8, fromCurrent);
			writeData(&rowSize, 4);
			// idziemy do przodu o ca�y wiersz i ostatni size...
			setFilePosition(rowSize + 4, fromCurrent);
		}
		_storedRowsCount++;
	}


// ----------------------------------------------------------------------




    int FileBin::fseterasedrow(bool overwrite , int testIndex) {
        if (feof(file)) return 1;
		if (!isOpened()) throw DTException(errNotOpened);
        if (fgetc(file) != '\n') return 1; // wczytuje '\n'
        if (feof(file)) return 1;
        if (_verMaj && _verMaj < '3') return 1;
        int siz1 , siz2;
        long pos;
        // Najpierw sprawdza czy zgadza si� rozmiar...
        fread(&siz1 , 4 , 1 , file); // size
        pos = ftell(file); // Pozycja zaraz za pierwszym SIZE'em
        fseek(file , siz1 , SEEK_CUR);
        fread(&siz2 , 4 , 1 , file); // size2
        if (feof(file) || siz1 != siz2) return 1; // Oba rozmiary nie s� zgodne!!!
        // Sprawdzamy index je�li jest taka potrzeba
        if (testIndex) {
            fseek(file , pos + 4 , SEEK_SET); // Czytamy DSize
            int dSize=0;
            fread(&dSize , 4 , 1 , file);
            if (dSize >= 8) { // Potrzebujemy r�wnie� flag�...
                int dFlag = 0;
                fread(&dFlag , 4 , 1 , file);
                if (dFlag & DT_BIN_DFID) {
                    int dID = 0;
                    fread(&dID , 4 , 1 , file);
                    if (dID != DT_UNMASKID(testIndex)) return 1; // Z�y index!
                }
            }
        }
        if (this->_storedRowsCount != -1 && this->_storedRowsCount) { // A mo�e ju� jest ustawione?
            fseek(file , pos , SEEK_SET); // Wracamy do pozycji
            int oldFlag = 0;
            fread(&oldFlag , 4 , 1 , file);
            if (oldFlag == -1) return 0;
            this->_storedRowsCount --;
        } else this->_storedRowsCount = -1;

        fseek(file , pos , SEEK_SET); // Wracamy do pozycji
        int newFlag = -1;
		if (fwrite(&newFlag  , 4 , 1 , file) != 1) // Zapisujemy flag�
			goto writefailed;
        if (overwrite) {
            // Zapisujemy 
            char * buff = new char [100];
            memset(buff , 0 , 100);
            size_t size = siz1 - 4;
            size_t i = 0;
            while (i < size) {
                if (!fwrite(buff , min(100 , size-i) , 1 , file)) break;
                i+=min(100 , size-i);
            }
            delete [] buff;
        }
        // Zaznaczamy flage w deskryptorze
        fseek(file , 7 , SEEK_SET);
        int flag;
        fread(&flag , 4 , 1 , file);
        flag |= DT_BINTF_FRAGMENTED;
        fseek(file , 7 , SEEK_SET);
		if (fwrite(&flag , 4 , 1, file) != 1)
			goto writefailed;
        fseek(file , pos + siz1 + 4 , SEEK_SET); // Skaczemy dalej
        return 0;
writefailed:
	  this->write_failed = true;
	  return 2;

    }



// READ




	int FileBin::ffindnextrow() { // przechodzi do nast�pnej linijki (w razie gdy freadrow wywali b��d)
		if (_verMaj <= '1' || feof(file))
			return 1;
		//size_t filesize = _filelength(file->_file);
		while (!feof(file)) {
			// szukamy '\n'
			if (getc(file) == '\n') {
				fseek(file , -1 , SEEK_CUR);
				return 0; // znalaz�em potencjalny nowy wpis... freadrow zajmie si� sprawdzaniem...
			}
		}
		return 1;
	}


    int FileBin::readrows() {
//      int pos = ftell(file);
//      freadsize();
//      fseek(file , pos , SEEK_SET);
        table->clearrows();
        int i;
        while (1 && !feof(file))
        { 
            i = table->addrow(0);
			while (1) {
				table->rows[i]->pos = ftell(file);
				if (freadrow(i)) {
					if (!ffindnextrow())
						continue; // pr�bujemy nast�pny...
					else {
						table->deleterow(i);
						return 0;
					}
				} else {
					break; // jest ok, nast�pny...
				}
			}
		
		}
        return 0;
    }




	} }