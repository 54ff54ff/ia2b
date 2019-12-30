/*========================================================================\
|: [Filename] vector.h                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the vector container                  :|
<------------------------------------------------------------------------*/

#ifndef HEHE_VECTOR_H
#define HEHE_VECTOR_H

#include "adtBase.h"

namespace _54ff
{

template <class T, class Alloc = Allocator<T>>
class Vector
{
public:
	Vector() { init(); }
	Vector(size_t n, const T& elem): s(n), cap(n)
		{ collect(); for(size_t i = 0; i < n; ++i) alloc.construct(getPtr(i), elem); }
	~Vector() { clean(); }

	/*====================================*/

	Vector(const Vector& v): cap(v.s)
		{ reset(); collect(); copy(v); }

	Vector(Vector&& v) { move(::move(v)); }

	Vector& operator=(const Vector& v)
		{  clear(); if(size_t c = v.s + 1; cap < c) { release(); cap = c; collect(); } copy(v); return *this; }

	Vector& operator=(Vector&& v)
		{ clean(); move(::move(v)); return *this; }

	/*====================================*/

	template <class TT>
	class iterator_base
	{
	friend class Vector;

	public:
		iterator_base() {}

		TT& operator* ()const { return *node; }
		TT* operator->()const { return  node; }

		iterator_base& operator++() { ++node; return *this; }
		iterator_base& operator--() { --node; return *this; }
		iterator_base  operator++(int) { return node++; }
		iterator_base  operator--(int) { return node--; }

		iterator_base& operator+=(size_t step) { node += step; return *this; }
		iterator_base& operator-=(size_t step) { node -= step; return *this; }
		iterator_base  operator+ (size_t step)const { return node + step; }
		iterator_base  operator- (size_t step)const { return node - step; }

		size_t operator-(const iterator_base& it)const { return node - it.node; }

		bool operator< (const iterator_base& it)const { return node <  it.node; }
		bool operator<=(const iterator_base& it)const { return node <= it.node; }
		bool operator> (const iterator_base& it)const { return node >  it.node; }
		bool operator>=(const iterator_base& it)const { return node >= it.node; }
		bool operator==(const iterator_base& it)const { return node == it.node; }
		bool operator!=(const iterator_base& it)const { return node != it.node; }

	private:
		iterator_base(TT* n): node(n) {}

	private:
		TT*  node;
	};

	using       iterator = iterator_base<      T>;
	using const_iterator = iterator_base<const T>;

	/*====================================*/
	
	size_t size()const { return s; }
	size_t capacity()const { return cap; }
	bool empty()const { return s == 0; }
	void clear() { clearInt(); reset(); }

	      iterator begin()      { return       iterator(getFirst()); }
	const_iterator begin()const { return const_iterator(getFirst()); }
	      iterator end  ()      { return       iterator(getLast()); }
	const_iterator end  ()const { return const_iterator(getLast()); }

	      T& front()      { return *getFirst(); }
	const T& front()const { return *getFirst(); }
	      T& back ()      { return *getPtr(s - 1); }
	const T& back ()const { return *getPtr(s - 1); }

	      T& operator[](size_t i)      { return *getPtr(i); }
	const T& operator[](size_t i)const { return *getPtr(i); }

	/*====================================*/

	void push_back(const T& value)
		{ checkExpand(); alloc.construct(getLast(), value);                  ++s; }
	void push_back(T&& value)
		{ checkExpand(); alloc.construct(getLast(), ::move(value));          ++s; }
	template <class... Args>
	void emplace_back(Args&&... args)
		{ checkExpand(); alloc.construct(getLast(), forward<Args>(args)...); ++s; }

	void insert(size_t pos, const T& value)  { insertInt(pos); alloc.construct(getPtr(pos), value); }
	void insert(size_t pos, T&& value)       { insertInt(pos); alloc.construct(getPtr(pos), ::move(value)); }
	template <class... Args>
	void emplace(size_t pos, Args&&... args) { insertInt(pos); alloc.construct(getPtr(pos), forward<Args>(args)...); }

	void pop_back() { alloc.destroy(getPtr(--s)); }
	void erase(size_t pos)
	{
		alloc.destroy(getPtr(pos));
		for(--s; pos < s; ++pos)
			alloc.construct(getPtr(pos), ::move(data[pos+1])), alloc.destroy(getPtr(pos+1));
	}

	void resize(size_t newS, const T& value)
	{
		if(s >= newS)
			while(s > newS)
				pop_back();
		else
		{
			if(newS > cap)
			{
				T* oldData = data;
				cap = newS; collect();
				moveAndDel(oldData);
			}
			for(; s < newS; ++s)
				alloc.construct(getPtr(s), value);
		}
	}

	void reserve(size_t newCap)
	{
		if(cap >= newCap) return;
		T* oldData = data;
		cap = newCap; collect();
		moveAndDel(oldData);
	}

	template <class Condition>
	void condErase(Condition cond)
	{
		size_t i = 0;
		for(size_t j = 0; j < s; ++j)
			if(cond(data[j]))
				alloc.construct(*getPtr(i++), ::move(data[j]));
		s = i;
	}

private:
	void insertInt(size_t pos)
	{
		if(s != cap)
			for(size_t i = s; i > pos; --i)
				alloc.construct(getPtr(i), ::move(data[i-1])), alloc.destroy(getPtr(i-1));
		else
		{
			T* oldData = data; expand();
			for(size_t i = 0;   i < pos; ++i)
				alloc.construct(getPtr(i),   ::move(oldData[i])), alloc.destroy(oldData + i);
			for(size_t i = pos; i < s;   ++i)
				alloc.construct(getPtr(i+1), ::move(oldData[i])), alloc.destroy(oldData + i);
			alloc.deallocate(oldData);
		}
		s += 1;
	}

	void clearInt()
		{ for(size_t i = 0; i < s; ++i) alloc.destroy(getPtr(i)); }

	void copy(const Vector& v)
		{ for(; s < v.s; ++s) alloc.construct(getPtr(s), v.data[s]); }

	void move(Vector&& v)
		{ data = v.data; s = v.s; cap = v.cap; v.init(); }

	void checkExpand()
	{
		if(s != cap) return;
		T* oldData = data; expand();
		moveAndDel(oldData);
	}

	void moveAndDel(T* oldData)
	{
		for(size_t i = 0; i < s; ++i)
			alloc.construct(getPtr(i), ::move(oldData[i])), alloc.destroy(oldData + i);
		alloc.deallocate(oldData);
	}

	T* getPtr  (size_t i)const { return data + i; }
	T* getFirst()const { return getPtr(0); }
	T* getLast ()const { return getPtr(s);  }

	size_t nextCap()const { return (cap * 3) / 2 + 1; }
	void expand() { cap = nextCap(); collect(); }
	void reset() { s = 0; }
	void init() { cap = 0; data = 0; reset(); }
	void clean() { clearInt(); release(); }
	void collect() { data = alloc.allocate(cap); }
	void release() { alloc.deallocate(data); }

private:
	T*      data;
	size_t  s, cap;
	Alloc   alloc;
};

}

#endif