/*
 * tools.c: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * vdr: Id: tools.c 2.0 2008/03/05 17:23:47 kls Exp
 */

#include "tools.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>


//// cListObject
cListObject::cListObject(void)
{
  prev = next = NULL;
}

cListObject::~cListObject()
{
}

void cListObject::Append(cListObject *Object)
{
  next = Object;
  Object->prev = this;
}

void cListObject::Insert(cListObject *Object)
{
  prev = Object;
  Object->next = this;
}

void cListObject::Unlink(void)
{
  if (next)
     next->prev = prev;
  if (prev)
     prev->next = next;
  next = prev = NULL;
}

int cListObject::Index(void) const
{
  cListObject *p = prev;
  int i = 0;

  while (p) {
        i++;
        p = p->prev;
        }
  return i;
}

//// cListBase
cListBase::cListBase(void)
{
  objects = lastObject = NULL;
  count = 0;
}

cListBase::~cListBase()
{
  Clear();
}

void cListBase::Add(cListObject *Object, cListObject *After)
{
  if (After && After != lastObject) {
     After->Next()->Insert(Object);
     After->Append(Object);
     }
  else {
     if (lastObject)
        lastObject->Append(Object);
     else
        objects = Object;
     lastObject = Object;
     }
  count++;
}

void cListBase::Ins(cListObject *Object, cListObject *Before)
{
  if (Before && Before != objects) {
     Before->Prev()->Append(Object);
     Before->Insert(Object);
     }
  else {
     if (objects)
        objects->Insert(Object);
     else
        lastObject = Object;
     objects = Object;
     }
  count++;
}

void cListBase::Del(cListObject *Object, bool DeleteObject)
{
  if (Object == objects)
     objects = Object->Next();
  if (Object == lastObject)
     lastObject = Object->Prev();
  Object->Unlink();
  if (DeleteObject) {
     delete Object;
     count--;
  }
}

void cListBase::Move(int From, int To)
{
  Move(Get(From), Get(To));
}

void cListBase::Move(cListObject *From, cListObject *To)
{
  if (From && To) {
     if (From->Index() < To->Index())
        To = To->Next();
     if (From == objects)
        objects = From->Next();
     if (From == lastObject)
        lastObject = From->Prev();
     From->Unlink();
     if (To) {
        if (To->Prev())
           To->Prev()->Append(From);
        From->Append(To);
        }
     else {
        lastObject->Append(From);
        lastObject = From;
        }
     if (!From->Prev())
        objects = From;
     }
}

void cListBase::Clear(void)
{
  while (objects) {
        cListObject *object = objects->Next();
        delete objects;
        objects = object;
        }
  objects = lastObject = NULL;
  count = 0;
}

cListObject *cListBase::Get(int Index) const
{
  if (Index < 0)
     return NULL;
  cListObject *object = objects;
  while (object && Index-- > 0)
        object = object->Next();
  return object;
}

static int CompareListObjects(const void *a, const void *b)
{
  const cListObject *la = *(const cListObject **)a;
  const cListObject *lb = *(const cListObject **)b;
  return la->Compare(*lb);
}

void cListBase::Sort(void)
{
  int n = Count();
  cListObject *a[n];
  cListObject *object = objects;
  int i = 0;
  while (object && i < n) {
        a[i++] = object;
        object = object->Next();
        }
  qsort(a, n, sizeof(cListObject *), CompareListObjects);
  objects = lastObject = NULL;
  for (i = 0; i < n; i++) {
      a[i]->Unlink();
      count--;
      Add(a[i]);
      }
}

//// packetqueue
PacketQueue::PacketQueue()
{
	pthread_mutex_init(&mutex, NULL);
}

PacketQueue::~PacketQueue()
{
	while (queue.begin() != queue.end()) 
	{
		delete[] queue.front();
		queue.pop_front();
	}
}

void PacketQueue::push(uint8_t* data)
{
	pthread_mutex_lock(&mutex);

	queue.push_back(data);

	pthread_mutex_unlock(&mutex);
}

uint8_t* PacketQueue::pop()
{
	uint8_t* retval;

	pthread_mutex_lock(&mutex);

	retval = queue.front();
	queue.pop_front();

	pthread_mutex_unlock(&mutex);
	
	return retval;
}

size_t PacketQueue::size()
{
	return queue.size();
}


////
/* Max 24 bit */
uint32_t getbits(const uint8_t* buf, uint32_t offset, uint8_t len)
{
	const uint8_t* a = buf + (offset / 8);
	uint32_t retval = 0;
	uint32_t mask = 1;

	retval = ((*(a)<<8) | *(a+1));
	mask <<= len;

	if (len > 8) {
		retval <<= 8;
		retval |= *(a+2);
		len -= 8;
	}
	if (len > 8) {
		retval <<= 8;
		retval |= *(a+3);
		len -= 8;
	}
	if (len > 8) {
		uint64_t tmp = retval << 8;
		tmp |= *(a+4);
		tmp >>= ((8-(offset%8)) + (8-(len)));
		return tmp & (mask -1);
	}

	retval >>= ((8-(offset%8)) + (8-len));
	return retval & (mask -1);
}

uint32_t * simple_resize32(uint8_t * origin, uint32_t * colors, int nb_colors, int ox, int oy, int dx, int dy)
{
	uint32_t  *cr, *l;
	int i, j, k, ip;

	cr = (uint32_t *) malloc(dx * dy * sizeof(uint32_t));

	if(cr == NULL) 
	{
		printf("Error: malloc\n");
		return NULL;
	}
	
	l = cr;

	for(j = 0; j < dy; j++, l += dx)
	{
		uint8_t * p = origin + (j*oy/dy*ox);
		
		for(i = 0, k = 0; i < dx; i++, k++) 
		{
			ip = i*ox/dx;
			
			int idx = p[ip];
			if(idx < nb_colors)
				l[k] = colors[idx];
		}
	}
	
	return(cr);
}

