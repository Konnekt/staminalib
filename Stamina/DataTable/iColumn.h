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

#pragma once
#ifndef __DT_ICOLUMN__
#define __DT_ICOLUMN__


#include "iRow.h"
#include "..\Object.h"
#include "..\ObjectPtr.h"
#include "..\StringSTL.h"

namespace Stamina { namespace DT {

	class iColumn: public iSharedObject {
	public:

		STAMINA_OBJECT_CLASS_VERSION(DT::iColumn, iSharedObject, Version(1, 0, 0, 0));

		iColumn* cloneColumn() {
			return (iColumn*)this->cloneObject();
		}

		inline enColumnType getType() const {
			return (enColumnType) (_type & ctypeMask);
		}
		inline enColumnFlag getFlags() const {
			return (enColumnFlag) _type;
		}

		inline tColId getId() const {
			return _id;
		}

		inline unsigned int getIndex() const {
			return _index;
		}

		inline const AString& getName() const {
			return _name;
		}

		inline bool isIdUnique() const {
			return (getId() & colIdUniqueFlag) != 0;
		}

		/** Returns true if type occupies only 4 bytes and can be stored directly (without creating objects)
		*/
		inline bool isStaticType() const {
			return this->getType() == ctypeInt;
		}


		bool hasFlag(enColumnFlag flag) const {
			return (_type & flag) != 0;
		}

		virtual void setFlag(enColumnFlag flag, bool setting) {
			flag = (enColumnFlag)(flag & ~ctypeMask);
			if (setting)
				_type = _type | flag;
			else
				_type = (enColumnType) (_type & (~flag));
		}

		bool isUndefined() const {
			return _type == ctypeUnknown;
		}

		// get / set

		/** Resets the value */
		virtual void reset(iRow* row) const = 0;

		virtual int getInt(const iRow* row, GetSet flags = gsNone) const { return 0; }
		virtual __int64 getInt64(const iRow* row, GetSet flags = gsNone) const { return 0; }
		virtual double getDouble(const iRow* row, GetSet flags = gsNone) const { return 0; }
		virtual String getString(const iRow* row, GetSet flags = getCopy) const { return String(); }
		virtual ByteBuffer getBin(const iRow* row, GetSet flags = getCopy) const { return ByteBuffer(); }

		virtual bool setInt(iRow* row, int, GetSet flags = gsNone) const { return false; }
		virtual bool setInt64(iRow* row, __int64, GetSet flags = gsNone) const { return false; }
		virtual bool setDouble(iRow* row, double, GetSet flags = gsNone) const { return false; }
		virtual bool setString(iRow* row, const StringRef&, GetSet flags = gsNone) const { return false; }
		virtual bool setBin(iRow* row, const ByteBuffer&, GetSet flags = gsNone) const { return false; }

		virtual bool convertible(enColumnType type, bool from) const {return false;}

	protected:

		virtual void cloneMembers(const iObject* a) {
			BaseClass::cloneMembers(a);
			iColumn* b = static_cast<iColumn*>( const_cast<iObject*>( a ) );
			this->_type = b->_type;
			this->_id = b->_id;
			this->_name = b->_name;
			this->_index = b->_index;
		}

		enColumnType _type;
		tColId _id;
        unsigned int _index;
		AString _name;  // tekstowy identyfikator

		

	};


	typedef SharedPtr<iColumn> oColumn;

	STAMINA_REGISTER_CLASS_VERSION(iColumn);

} }

#endif