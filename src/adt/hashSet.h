/*========================================================================\
|: [Filename] hashSet.h                                                  :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Define and implement the hash set container                :|
<------------------------------------------------------------------------*/

#ifndef HEHE_HASHSET_H
#define HEHE_HASHSET_H

#include "vector.h"
#include <vector>

namespace _54ff
{

//got from MiniSAT 2.2.0, in mtl/Map.h, at line 48, 49
constexpr size_t numPrimes = 25;
constexpr size_t primes[numPrimes] = { 31,
                                       73,
                                      151,
                                      313,
                                      643,
                                     1291,
                                     2593,
                                     5233,
                                    10501,
                                    21013,
                                    42073,
                                    84181,
                                   168451,
                                   337219,
                                   674701,
                                  1349473,
                                  2699299,
                                  5398891,
                                 10798093,
                                 21596719,
                                 43193641,
                                 86387383,
                                172775299,
                                345550609,
                                691101253 };

inline size_t getHashSizeMSat(size_t s)
{
	size_t i = 0;
	for(; i < numPrimes && primes[i] < s; ++i);
	return primes[i];
}

inline size_t getHashSizeRic(size_t s)
{
	if (s <         8) return       7;
	if (s <        16) return      13;
	if (s <        32) return      31;
	if (s <        64) return      61;
	if (s <       128) return     127;
	if (s <       512) return     509;
	if (s <      2048) return    1499;
	if (s <      8192) return    4999;
	if (s <     32768) return   13999;
	if (s <    131072) return   59999;
	if (s <    524288) return  100019;
	if (s <   2097152) return  300007;
	if (s <   8388608) return  900001;
	if (s <  33554432) return 1000003;
	if (s < 134217728) return 3000017;
	if (s < 536870912) return 5000011;
	return 7000003;
}

inline size_t getHashSize(size_t s)
{
//	return getHashSizeMSat(s);
	return getHashSizeRic (s);
}

template <class T, class Hash = Hasher<T>, class Equal = EqualTo<T>>
class HashSet
{
public:
	HashSet(): bucket(0), numBucket(0) {}
	HashSet(size_t n): numBucket(getHashSize(n)) { bucket = new Vector<T>[numBucket]; }
	~HashSet() { delete []bucket; }

	/*====================================*/

	//implement if really needed
	HashSet(const HashSet&) = delete;
	HashSet(HashSet&&) = delete;
	HashSet& operator=(const HashSet&) = delete;
	HashSet& operator=(HashSet&&) = delete;

	/*====================================*/

	//only for reading
	class iterator
	{
	friend class HashSet;

	public:
		iterator() {}

		const T& operator* ()const { return  host->bucket[idxOfBkt][idxInBkt]; }
		const T* operator->()const { return &host->bucket[idxOfBkt][idxInBkt]; }

		iterator& operator++()
		{
			if(++idxInBkt == host->numInBucket[idxOfBkt])
				for(idxInBkt = 0, ++idxOfBkt;
				    idxOfBkt < host.numBucket() && host->numInBucket[idxOfBkt] == 0;
				    ++idxOfBkt);
			return *this;
		}
		iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

		bool operator==(const iterator& it)const { return idxOfBkt == it.idxOfBkt &&
		                                                  idxInBkt == it.idxInBkt; }
		bool operator!=(const iterator& it)const { return !(*this == it); }

	private:
		iterator(const HashSet* h, size_t iob, size_t iib = 0)
		: host(h), idxOfBkt(iob), idxInBkt(iib) {}

	private:
		const HashSet*  host;
		size_t          idxOfBkt;
		size_t          idxInBkt;
	};

	/*====================================*/

	iterator begin()const
	{
		assert(bucket);
		size_t i = 0;
		for(; i < numBucket; ++i)
			if(!bucket[i].empty())
				break;
		return iterator(this, i);
	}
	iterator end()const { return iterator(this, numBucket); }

	bool empty()const
	{
		for(size_t i = 0; i < numBucket; ++i)
			if(!bucket[i].empty())
				return false;
		return true;
	}

	size_t size()const
	{
		size_t s = 0;
		for(size_t i = 0; i < numBucket; ++i)
			s += bucket[i].size();
		return true;
	}

	size_t numOfBucket()const { return numBucket; }
	size_t numInBucket(size_t b)const { return bucket[b].size(); }

	void clear()
	{
		for(size_t i = 0; i < numBucket; ++i)
			bucket[i].clear();
	}

	/*====================================*/

	iterator find(const T& value)const
	{
		const size_t b = getHashIdx(value);
		for(size_t i = 0, s = bucket[b].size(); i < s; ++i)
			if(equal(value, bucket[b][i]))
				return iterator(this, b, i);
		return end();
	}

	iterator exist(const T& value)const
	{
		const size_t b = getHashIdx(value);
		for(size_t i = 0, s = bucket[b].size(); i < s; ++i)
			if(equal(value, bucket[b][i]))
				return true;
		return false;
	}

	using insResult = Pair<iterator, bool>;
	insResult insert(const T& value)
	{
		const size_t b = getHashIdx(value);
		const size_t s = bucket[b].size();
		for(size_t i = 0; i < s; ++i)
			if(equal(value, bucket[b][i]))
				return { iterator(this, b, i), false };
		bucket[b].push_back(value);
		return { iterator(this, b, s), true };
	}

	insResult insert(T&& value)
	{
		const size_t b = getHashIdx(value);
		const size_t s = bucket[b].size();
		for(size_t i = 0; i < s; ++i)
			if(equal(value, bucket[b][i]))
				return { iterator(this, b, i), false };
		bucket[b].push_back(move(value));
		return { iterator(this, b, s), true };
	}

	template <class... Args>
	insResult emplace(Args&&... args) { return insert(T(forward<Args>(args)...)); }

private:
	size_t getHashIdx(const T& value)const { return hash(value) % numBucket; }

private:
	Vector<T>*  bucket;
	size_t      numBucket;
	Hash        hash;
	Equal       equal;
};

}

#endif