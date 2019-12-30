/*========================================================================\
|: [Filename] deque.h                                                    :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the deque container                   :|
<------------------------------------------------------------------------*/

#ifndef HEHE_DEQUE_H
#define HEHE_DEQUE_H
 
#include "adtBase.h"

namespace _54ff
{

template <class T, class Alloc = Allocator<T>>
class Deque
{
//This implementation is enhanced from the Queue structure of MiniSAT 2.2.0
/* Two cases
1. first <= last
   begin   first   last   end
     |       |       |     |
     |       ---------     |
     |          size       |
     -----------------------
             capacity

2. first > last
   begin   last   first   end
     |       |      |      |
     ---------      --------
     |     |          |    |
     |     ------------    |
     |         size        |
     -----------------------
             capacity
*/

public:
	Deque()  { init(); }
	~Deque() { clean(); }

	/*====================================*/

	Deque(const Deque& d): cap(d.size()+1)
		{ reset(); collect(); copy(d); }

	Deque(Deque&& d) { move(::move(d)); }

	Deque& operator=(const Deque& d)
		{ clear(); if(size_t c = d.size() + 1; cap < c) { release(); cap = c; collect(); } copy(d); return *this; }

	Deque& operator=(Deque&& d)
		{ clean(); move(::move(d)); return *this; }

	/*====================================*/

	//only for reading
	class iterator
	{
	friend class Deque;

	public:
		iterator() {}

		const T& operator* ()const { return *node; }
		const T* operator->()const { return  node; }

		iterator& operator++() { if(++node == host->getPtr(host->cap)) node = host->getPtr(0);             return *this; }
		iterator& operator--() { if(node-- == host->getPtr(0))         node = host->getPtr(host->cap - 1); return *this; }
		iterator  operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
		iterator  operator--(int) { iterator tmp(*this); --(*this); return tmp; }

		bool operator==(const iterator& it)const { return node == it.node; }
		bool operator!=(const iterator& it)const { return node != it.node; }

	private:
		iterator(const T* n, const Deque* h): node(n), host(h) {}

	protected:
		const T*      node;
		const Deque*  host;
	};

	/*====================================*/

	size_t size()const { return first <= last ? last - first : (cap - first) + last; }
	size_t capacity()const { return cap; }
	bool empty()const { return first == last; }
	void clear() { clearInt(); reset(); }

	iterator begin()const { return iterator(getFirst(), this); }
	iterator end  ()const { return iterator(getLast(),  this); }

	      T& front()      { return *getFirst(); }
	const T& front()const { return *getFirst(); }
	      T& back ()      { return *getPtr(last == 0 ? cap - 1 : last - 1); }
	const T& back ()const { return *getPtr(last == 0 ? cap - 1 : last - 1); }

	      T& operator[](size_t i)      { return *getPtr((i += first) >= cap ? i - cap : i); }
	const T& operator[](size_t i)const { return *getPtr((i += first) >= cap ? i - cap : i); }

	/*====================================*/

	void reserve(size_t newCap)
	{
		if(cap >= newCap) return;
		T* oldData = data;
		size_t oldCap = cap;
		cap = newCap; collect();
		size_t i = 0;
		if(first <= last)
			for(size_t j = first; j < last;   ++j, ++i)
				alloc.construct(getPtr(i), ::move(oldData[j])), alloc.destroy(oldData + j);
		else
		{
			for(size_t j = first; j < oldCap; ++j, ++i)
				alloc.construct(getPtr(i), ::move(oldData[j])), alloc.destroy(oldData + j);
			for(size_t j = 0;     j < last;   ++j, ++i)
				alloc.construct(getPtr(i), ::move(oldData[j])), alloc.destroy(oldData + j);
		}
		first = 0; last = i;
		alloc.deallocate(oldData);
	}

	void push_back(const T& value)
		{ alloc.construct(getLast(), value);                  checkPushBack(); checkExpand(); }
	void push_back(T&& value)
		{ alloc.construct(getLast(), ::move(value));          checkPushBack(); checkExpand(); }
	template <class... Args>
	void emplace_back(Args&&... args)
		{ alloc.construct(getLast(), forward<Args>(args)...); checkPushBack(); checkExpand(); }

	void push_front(const T& value)
		{ checkPushFront(); alloc.construct(getFirst(), value);                  checkExpand(); }
	void push_front(T&& value)
		{ checkPushFront(); alloc.construct(getFirst(), ::move(value));          checkExpand(); }
	template <class... Args>
	void emplace_front(Args&&... args)
		{ checkPushFront(); alloc.construct(getFirst(), forward<Args>(args)...); checkExpand(); }

	void pop_back()
		{ checkPopBack(); alloc.destroy(getLast()); }
	void pop_front()
		{ alloc.destroy(getFirst()); checkPopFront(); }

private:
	void clearInt()
	{
		if(first <= last)
			for(size_t i = first; i < last; ++i)
				alloc.destroy(getPtr(i));
		else
		{
			for(size_t i = first; i < cap;  ++i)
				alloc.destroy(getPtr(i));
			for(size_t i = 0;     i < last; ++i)
				alloc.destroy(getPtr(i));
		}
	}

	void copy(const Deque& d)
	{
		if(d.first <= d.last)
			for(size_t i = d.first; i < d.last; ++i, ++last)
				alloc.construct(getPtr(last), d.data[i]);
		else
		{
			for(size_t i = d.first; i < d.cap;  ++i, ++last)
				alloc.construct(getPtr(last), d.data[i]);
			for(size_t i = 0;       i < d.last; ++i, ++last)
				alloc.construct(getPtr(last), d.data[i]);
		}
	}

	void move(Deque&& d)
	{
		data  = d.data;
		cap   = d.cap;
		first = d.first;
		last  = d.last;
		d.init();
	}

	void checkExpand()
	{
		if(first != last) return;
		T* oldData = data;
		size_t oldCap = cap;
		expand();
		size_t i = 0;
		for(size_t j = first; j < oldCap; ++j, ++i)
			alloc.construct(getPtr(i), ::move(oldData[j])), alloc.destroy(oldData + j);
		for(size_t j = 0;     j < last;   ++j, ++i)
			alloc.construct(getPtr(i), ::move(oldData[j])), alloc.destroy(oldData + j);
		first = 0; last = oldCap;
		alloc.deallocate(oldData);
	}

	T* getPtr  (size_t i)const { return data + i; }
	T* getFirst()const { return getPtr(first); }
	T* getLast ()const { return getPtr(last);  }

	size_t nextCap()const { return (cap * 3) / 2 + 1; }
	void expand() { cap = nextCap(); collect(); }
	void reset(size_t v = 0) { first = last = v; }
	void init() { cap = 1; collect(); reset(); }
	void clean() { clearInt(); release(); }
	void collect() { data = alloc.allocate(cap); }
	void release() { alloc.deallocate(data); }

	void checkPushBack()  { if(++last    == cap) last  = 0; }
	void checkPopBack()   { if(  last--  == 0)   last  = cap - 1; }
	void checkPushFront() { if(  first-- == 0)   first = cap - 1; }
	void checkPopFront()  { if(++first   == cap) first = 0; }

private:
	T*      data;
	size_t  cap;
	size_t  first;
	size_t  last;
	Alloc   alloc;
};

}

#endif
