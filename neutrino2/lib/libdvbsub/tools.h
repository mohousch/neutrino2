/*
 * tools.h: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: tools.h,v 1.1 2009/02/23 19:46:44 rhabarber1848 Exp $
 */

#ifndef __TOOLS_H
#define __TOOLS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <list>
#include <inttypes.h>
#include <pthread.h>


////
uint32_t getbits(const uint8_t* buf, uint32_t offset, uint8_t len);
uint32_t * simple_resize32(uint8_t * origin, uint32_t * colors, int nb_colors, int ox, int oy, int dx, int dy);

////
class cListObject 
{
	private:
		cListObject *prev, *next;
	public:
		cListObject(void);
		virtual ~cListObject();
		virtual int Compare(const cListObject /*&ListObject*/) const { return 0; }
	    	///< Must return 0 if this object is equal to ListObject, a positive value
	    	///< if it is "greater", and a negative value if it is "smaller".
		void Append(cListObject *Object);
		void Insert(cListObject *Object);
		void Unlink(void);
		int Index(void) const;
		cListObject *Prev(void) const { return prev; }
		cListObject *Next(void) const { return next; }
};

class cListBase 
{
	protected:
		cListObject *objects, *lastObject;
		cListBase(void);
	 	int count;
	 	
	public:
		virtual ~cListBase();
		void Add(cListObject *Object, cListObject *After = NULL);
		void Ins(cListObject *Object, cListObject *Before = NULL);
		void Del(cListObject *Object, bool DeleteObject = true);
		virtual void Move(int From, int To);
		void Move(cListObject *From, cListObject *To);
		virtual void Clear(void);
		cListObject *Get(int Index) const;
		int Count(void) const { return count; }
		void Sort(void);
};

template<class T> class cList : public cListBase 
{
	public:
		T *Get(int Index) const { return (T *)cListBase::Get(Index); }
		T *First(void) const { return (T *)objects; }
		T *Last(void) const { return (T *)lastObject; }
		T *Prev(const T *object) const { return (T *)object->cListObject::Prev(); } // need to call cListObject's members to
		T *Next(const T *object) const { return (T *)object->cListObject::Next(); } // avoid ambiguities in case of a "list of lists"
};

////
class PacketQueue 
{
	public:
		PacketQueue();
		~PacketQueue();
		void push(uint8_t* data);
		uint8_t* pop();
		size_t size();

	private:
		std::list<uint8_t*> queue;
		pthread_mutex_t mutex;
};

#endif //__TOOLS_H

