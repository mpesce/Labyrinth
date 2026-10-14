/* Copyright (C) 1992-1998 The Geometry Center
 * Copyright (C) 1998-2000 Stuart Levy, Tamara Munzner, Mark Phillips
 *
 * This file is part of Geomview.
 * 
 * Geomview is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * Geomview is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Geomview; see the file COPYING.  If not, write
 * to the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139,
 * USA, or visit http://www.gnu.org.
 */
#ifndef _QV_DICT_
#define _QV_DICT_

#include <QvBasic.h>
#include <QvString.h>
#include <QvPList.h>

class QvDictEntry {
  private:
    QvULong		key;
    void *		value;
    QvDictEntry *	next;
    QvDictEntry(QvULong k, void *v)	{ key = k; value = v; };

friend class QvDict;
};

class QvDict {
  public:
    QvDict( int entries = 251 );
    ~QvDict();
    void	clear();
    QvBool	enter(QvULong key, void *value);
    QvBool	find(QvULong key, void *&value) const;
    QvBool	remove(QvULong key);

  private:
    int			tableSize;
    QvDictEntry *	*buckets;
    QvDictEntry *&	findEntry(QvULong key) const;
};

#endif /* _QV_DICT_ */
