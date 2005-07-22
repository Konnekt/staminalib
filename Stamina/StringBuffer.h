/*
 *  Stamina.LIB
 *  
 *  Please READ /License.txt FIRST! 
 * 
 *  Copyright (C)2003,2004,2005 Rafa� Lindemann, Stamina
 *
 *  $Id$
 */

#ifndef __STAMINA_STRINGBUFFER__
#define __STAMINA_STRINGBUFFER__

#pragma once

#include "Assert.h"
#include "Memory.h"

namespace Stamina {

	/** Character buffer template.
	Implements basic character buffer operations without regard to type, size or codepage.

	Supports:
	 - cheap referencing - operations on external buffer of unknown size
	 - data discard - ability to discard buffer's data without freeing the memory
	 - up to 0x0FFFFFFF (268.435.455) bytes of length
	 - additional _active flag/bit
	 - always makes room for \0 char at the string's end. All buffer and data sizes don't count the terminating null character and neither You should do that!

	*/
	template <typename CHAR>
	class StringBuffer {
	public:
		const static unsigned int pooledBufferSize = 64;
		const static unsigned int maxBufferSize = 0x0FFFFFFF;
		const static unsigned int lengthUnknown = 0x0FFFFFFF;
		const static unsigned int wholeData = 0xFFFFFFFF;
	public:
		inline StringBuffer(): _size(0), _buffer(0), _active(0), _length(0) {
		}

		inline StringBuffer(unsigned int initialSize): _size(0), _buffer(0), _active(0), _length(0) {
			resize(initialSize, 0);
		}

		inline ~StringBuffer() {
			freeBuffer();
		}

		inline void setActive(bool active) {
			_active = active;
		}
		inline bool isActive() {
			return _active;
		}

		/** Creates "cheap reference" - provided buffer will replace the one currently in use, until modification occurs.
		*/
		inline void assignCheapReference(const CHAR* data, unsigned int length = lengthUnknown) {
			this->reset();
			this->_flag = true;
			this->_buffer = (CHAR*)data;
			this->_length = length;
		}

		/** Makes a copy of data */
		inline void assign(const CHAR* data, unsigned int size) {
			this->makeRoom(size, 0);
			copy(_buffer, data, size);
			this->_length = size;
			markValid();
		}

		/** Calculates the number of bytes needed to store @a newSize of data. It only expands current buffer size. */
		inline unsigned int calculateRoom(unsigned int newSize) {
			unsigned int allocSize = getBufferSize();
			if (newSize > allocSize) {
				if (allocSize < (maxBufferSize/2)) 
					allocSize *= 2;
			}
			return allocSize > newSize ? allocSize : newSize;
		}

		/** Ensures that there's enough space in the buffer, resizing the buffer if necessary. See resize() for more information.

		@param keepData - the same as in resize()
		@return Returns true if buffor was (re)allocated
		*/
		inline bool makeRoom(unsigned int newSize, unsigned int keepData = wholeData) {
			if (hasOwnBuffer()) {
				if (newSize > getBufferSize()) {
					this->resize(calculateRoom(newSize), keepData);
					return true;
				}
			} else { // jak nie ma bufora, zawsze alokujemy nowy
				resize(newSize, keepData);
				return true;
			}
			return false;
		}

		/** If the buffer is a reference - make a copy of it 
		@param keepData - the same as in resize()
		*/
		inline void makeUnique(unsigned int keepData = wholeData) {
			if (isReference()) {
				if (keepData > getLength()) keepData = getLength();
				resize(keepData, keepData);
			}
		}

		/** Returns number of used bytes in the buffer */
		inline unsigned int getLength() {
			if (_length == lengthUnknown) {
				_length = 0;
				if (!isEmpty() && isValid()) {
					CHAR* ch = _buffer;
					while (*(ch++)) ++_length;
				}
			}
			return _length;
		}

		/** Appends data to the end of the buffer
		*/
		inline void append(const CHAR* data, unsigned int dataSize) {
			if (dataSize == 0) return;
			makeRoom(getLength() + dataSize);
			_length += dataSize;
			copy(_buffer + getLength(), data, dataSize);
			markValid();
		}

		/** Prepends data to the buffer 
		*/
		inline void prepend(const CHAR* data, unsigned int dataSize) {
			if (dataSize == 0) return;
			if (isValid()) {
				moveRight(0, dataSize); // wywoluje makeroom
			} else {
				makeRoom(dataSize, 0);
			}
			_length += dataSize;
			copy(_buffer, data, dataSize);
			markValid();
		}

		/** Inserts data into the buffer. Fails if buffer is not valid.
		@param pos The position where to insert the data
		*/
		inline void insert(unsigned int pos, const CHAR* data, unsigned int dataSize) {
			if (dataSize == 0) return;
			if (isValid()) {
				moveRight(pos, dataSize); // wywoluje makeroom
			} else {
				makeRoom(dataSize, 0);
				pos = 0;
			}
			copy(_buffer + pos, data, dataSize);
			_length += dataSize;
			markValid();
		}

		inline void erase(unsigned int pos, unsigned int count = wholeData) {
			if (!isValid() || count == 0) return;
			if (count > getLength() || pos + count > getLength()) count = getLength() - pos;
			if (pos + count >= getLength()) {
				truncate(pos);
				return;
			}
			moveLeft(pos + count, count);
		}

		/** Truncates buffer data at the specified position. */
		inline void truncate(unsigned int pos) {
			if (!isValid()) return;
			if (pos > getLength()) pos = getLength();
			if (isReference()) {
				makeUnique(pos);
			}
			this->_length = pos;
			this->markValid();
		}


		/** Moves contents of the buffer to the left (optimized). Fails if buffer is not valid.
		@param start Position of the first character to move
		@param offset Offset of movement
		@param length The length of data to move
		@param truncate Truncates buffer after moved data
		*/
		void moveLeft(unsigned int start, unsigned int offset, unsigned int length = wholeData, bool truncate = true) {
			if (!isValid() || offset == 0) return;
			CHAR* from = _buffer;
			unsigned int dataLength = getLength();
			if (length > dataLength) length = dataLength;
			if (start < offset) {
				if (length > (offset - start)) {
					length -= (offset - start); // musimy skr�ci� d�ugo�� kopiowania o tyle, o ile p�niej je zaczynamy
				} else {
					length = 0;
				}
				start = offset;
			}
			if (start >= dataLength) {// nie ma sk�d ich przesuwa�
				if (truncate) 
					this->truncate(0);
				return;
			} 
			if (length + start > dataLength) {
				length = dataLength - start;
			}
			if (length == 0) {
				if (truncate) 
					this->truncate(start - offset);
				return;
			}
			if (isReference()) {
				// makeUnique bylby z�y bo nie rezerwowa�by bufora na przenoszenie
				resize(truncate ? start - offset + length : dataLength, start - offset); // kopiujemy tylko to co zostanie na pocz�tku
			}
			CHAR* to = _buffer;

			if (truncate/* || (length + start >= this->_length)*/) {
				this->_length = start - offset + length;
			} else {
				// skoro nic nie ucinamy - d�ugo�� pozostaje bez zmian. Przy zmianie z reference mog�a si� jednak zmieni�, wi�c przywracamy star�.
				this->_length = dataLength;
				if (getBuffer() != from) { // kopiujemy pozosta�o�ci
					copy(to + start - offset + length, from + start - offset + length, dataLength - (start + length - offset));
				}
			}

			from += start;
			to += start - offset;
			while (length--) {
				*to = *from;
				++to;
				++from;
			}
			markValid();
		}

		
		/** Moves contents of the buffer to the right (optimized). Fails if buffer is not valid.
		@param start Position of the first character to move
		@param offset Offset of movement
		@param length Length of data to move
		@param truncate Truncates data beyond moved data
		*/
		void moveRight(unsigned int start, unsigned int offset, unsigned int length = wholeData, bool truncate = true) {
			if (!isValid() || offset == 0) return;
			unsigned int dataLength = getLength();
			if (length > dataLength) length = dataLength;
			if (start > dataLength) return;
			if (length > dataLength - start)
				length = dataLength - start;

			CHAR* from = _buffer;
			if (isReference() && truncate) {
				makeRoom(start + offset + length, start + offset); // potrzebujmey tylko co ma by� na pocz�tku...
			} else {
				makeRoom(max(truncate ? 0 : dataLength, start + offset + length)); // potrzebujemy wszystko
				from = _buffer;
			}
			CHAR* to = _buffer;

			this->_length = max(truncate ? 0 : dataLength, start + offset + length);

            from += start + length;
			to += start + length + offset;

			while (length--) {
				--to;
				--from;
				*to = *from;
			}
			markValid();

		}

		// -- more internal

		/** Resets the buffer completely (frees all allocated memory) */
		inline void reset() {
			freeBuffer();
			_buffer = 0;
			_size = 0;
			_length = 0;
			_flag = false;
		}

		void resize(unsigned int newSize, unsigned int keepData = wholeData) {
			S_ASSERT(newSize <= maxBufferSize);
			if (keepData && newSize > 0 && this->isValid()) {
				if (keepData > getLength()) keepData = getLength();
				if (keepData > newSize) keepData = newSize;
				unsigned int size = newSize;
				CHAR* buffer = _alloc(size);
				copy(buffer, _buffer, keepData);
				freeBuffer();
				this->_buffer = buffer;
				this->_size = size;
				this->_flag = false;
				this->_length = keepData;
				markValid();
			} else {
				freeBuffer();
				unsigned int size = newSize;
				if (size > 0) {
					this->_buffer = _alloc(size);
				}
				this->_size = size;
				this->_flag = true; // discard
				this->_length = 0;
			}
		}

		/** Discards data in buffer, leaving the buffer allocated in memory so we can use it later. */
		inline void discard() {
			if (! this->isReference()) {
				_flag = true;
				this->_length = 0;
			} else {
				reset();
			}
		}

		// -- static helpers

		static inline void copy(CHAR* to, const CHAR* from, unsigned int count) {
			while (count--) {
				*to = *from;
				++to;
				++from;
			}
		}

		// getters

		/** Returns the size of an allocated buffer. If nothing is allocated (ie. data is being referenced) - returns 0 

		@warning Space for \0 character is automatically allocated and not reflected in the buffer's size. So getBuffer()[getBufferSize()] = 0 is perfectly legal.
		*/
		inline unsigned int getBufferSize() const {
			return _size;
		}

		inline CHAR* getBuffer() {
			return _buffer;
		}

		inline const CHAR* getString() {
			return isValid() ? _buffer : (CHAR*)L"";
		}

		//

		/** Returns true if data is valid (not discarded) */
		inline bool isValid() const {
			return (_flag == false && _size > 0) || isReference();
		}

		inline bool isReference() const {
			return _flag == true && _size == 0 && _buffer != 0;
		}

		inline bool isEmpty() const {
			return _buffer == 0 && _size == 0;
		}

		inline bool hasOwnBuffer() const {
			return _size > 0;
		}

	private:


		inline void freeBuffer() {
			if (hasOwnBuffer()) {
				_free(_buffer, getBufferSize());
			}
			_buffer = 0;
			_size = 0;
			_flag = false;
		}

		/** Marks data as valid (if it's not referenced) */
		inline void markValid() {
			if ( this->hasOwnBuffer() ) {
				_flag = false;
				if (this->_length != lengthUnknown)
					this->_buffer[this->_length] = 0;
			}
		}


		/** Returned memory block is always @a size + 1 of size */
		static CHAR* _alloc(unsigned int &size) {
			return Memory::allocBuffer<CHAR>(size);
		}
		static void _free(CHAR* buff, unsigned int size) {
			Memory::freeBuffer<CHAR>(buff, size);
		}

		CHAR* _buffer;
		bool _flag : 1;
		bool _active : 1;
		int _align1 : 2;
		unsigned int _size : 28;
		int _align2 : 4;
		unsigned int _length : 28;
	};


	template StringBuffer<char>;
	template StringBuffer<wchar_t>;


};

#endif