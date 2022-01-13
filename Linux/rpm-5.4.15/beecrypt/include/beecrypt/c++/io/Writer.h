/*
 * Copyright (c) 2004 X-Way Rights BV
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!\file Writer.h
 * \ingroup CXX_IO_m
 */

#ifndef _ABSTRACT_CLASS_BEE_IO_WRITER_H
#define _ABSTRACT_CLASS_BEE_IO_WRITER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Appendable.h"
using beecrypt::lang::Appendable;
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/io/Closeable.h"
using beecrypt::io::Closeable;
#include "beecrypt/c++/io/Flushable.h"
using beecrypt::io::Flushable;

namespace beecrypt {
	namespace io {
		/*!\ingroup CXX_IO_m
		 */
		class BEECRYPTCXXAPI Writer : public Object, public virtual Closeable, public virtual Flushable, public virtual Appendable
		{
		protected:
			Object* lock;

			Writer();
			Writer(Object& lock);

		public:
			virtual Writer& append(jchar c) throw (IOException);
			virtual Writer& append(const CharSequence& cseq) throw (IOException);

			virtual void write(const array<jchar>& cbuf) throw (IOException);
			virtual void write(const jchar* cbuf, jint off, jint len) throw (IOException) = 0;
			virtual void write(jint c) throw (IOException);
			virtual void write(const String& str) throw (IOException);
			virtual void write(const String& str, jint off, jint len) throw (IOException);
		};
	}
}

#endif

#endif
