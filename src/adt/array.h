/*========================================================================\
|: [Filename] array.h                                                    :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the array container                   :|
<------------------------------------------------------------------------*/

#ifndef HEHE_ARRAY_H
#define HEHE_ARRAY_H

#include <new>
#include <type_traits>
using namespace std;

namespace _54ff
{

template <class T, size_t... Ns> struct sArrInt1;
template <class T, size_t... Ns> using sArray1 = typename sArrInt1<T, Ns...>::type; //static multi-dimensional array
template <class T, size_t N, size_t... Ns>
struct sArrInt1<T, N, Ns...>
{
using type = sArrInt1<T, N, Ns...>;
private: using storage = sArray1<T, Ns...>;

public:
	      storage& operator[](size_t i)      { return array[i]; }
	const storage& operator[](size_t i)const { return array[i]; }

private:
	storage  array[N];
};
template <class T> struct sArrInt1<T> { using type = T; };

/*========================================================================*/

template <class T, size_t... Ns> using sArray = sArray1<T, Ns...>;

/*========================================================================*/

template <class T, size_t d>
class dArray1
{
static_assert(d > 0, "Zero dimension");
using storage = conditional_t<(d > 1), dArray1<T, d-1>, T>;

public:
	dArray1(): array(0) {}
	template <typename... Lengths>
	explicit dArray1(Lengths... L) { init(L...); }
	~dArray1() { delete []array; }

	//for these dynamic array, be sure to follow rule of 5
	//since we do not know the exact length of each sub-array during the copy
	//just prohibit it
	dArray1(const dArray1&) = delete;
	dArray1& operator=(const dArray1&) = delete;
	//just move the top pointer to the new container
	dArray1(dArray1&& da): array(da.array) { da.array = 0; }
	dArray1& operator=(dArray1&& da) { array = da.array; da.array = 0; return *this; }

	      storage& operator[](size_t i)      { return array[i]; }
	const storage& operator[](size_t i)const { return array[i]; }

	template <typename... Lengths>
	void init(unsigned s, Lengths... L) { static_assert(sizeof...(L) < d, "Too much dimensions");
	                                      array = new storage[s](); for(unsigned i = 0; i < s; ++i) array[i].init(L...); }
	void init(unsigned s)               { array = new storage[s](); }

	void reset() { delete []array; array = 0; }
	bool empty()const { return array == 0; }

private:
	storage*  array;
};

/*========================================================================*/

template <class T, size_t d> struct ptrInt2;
template <class T, size_t d> using ptr2 = typename ptrInt2<T, d>::type;
template <class T, size_t d>
struct ptrInt2
{
friend struct ptrInt2<T, d+1>;
using type = ptrInt2<T, d>;
private: using storage = ptr2<T, d-1>;

public:
	      storage& operator[](size_t i)      { return array[i]; }
	const storage& operator[](size_t i)const { return array[i]; }

	template <typename... Lengths>
	void init(unsigned s, Lengths... L) { static_assert(sizeof...(L) < d, "Too much dimensions");
	                                      array = new storage[s](); for(unsigned i = 0; i < s; ++i) array[i].init(L...); }
	void init(unsigned s)               { array = new storage[s](); }

	void reset() { delete []array; array = 0; }

protected:
	storage*  array;

	ptrInt2(): array(0) {}
	~ptrInt2() { delete []array; }
};
template <class T> struct ptrInt2<T, 0> { using type = T; };

template <class T, size_t d>
class dArray2 : public ptr2<T, d> //dynamic multi-dimensional array
{
static_assert(d > 0, "Zero dimension");

public:
	dArray2() {}
	template <typename... Lengths>
	explicit dArray2(Lengths... L) { ptr2<T, d>::init(L...); }
	~dArray2() {}

	dArray2(const dArray2&) = delete;
	dArray2& operator=(const dArray2&) = delete;

	dArray2(dArray2&& da) { ptr2<T, d>::array = da.array; da.array = 0; }
	dArray2& operator=(dArray2&& da) { ptr2<T, d>::array = da.array; da.array = 0; return *this; }
};

/*========================================================================*/

template <class T, size_t d>
class dArray3
{
static_assert(d > 0, "Zero dimension");

public:
	dArray3(): array(0) {}
	template <typename... Lengths>
	explicit dArray3(Lengths... L) { init(L...); }
	~dArray3() { delete []array; }

	dArray3(const dArray3&) = delete;
	dArray3& operator=(const dArray3&) = delete;

	dArray3(dArray3&& da): array(da.array), size(da.size) { da.array = 0; }
	dArray3& operator=(dArray3&& da) { array = da.array; da.array = 0; size = da.size; return *this; }

	template <typename... Lengths>
	      T& operator()(Lengths... L)      { static_assert(sizeof...(L) == d, "Wrong number of dimension"); return array[getIdx(L...)]; }
	template <typename... Lengths>
	const T& operator()(Lengths... L)const { static_assert(sizeof...(L) == d, "Wrong number of dimension"); return array[getIdx(L...)]; }

	template <typename... Lengths>
	void init(Lengths... L) { static_assert(sizeof...(L) == d, "Wrong number of dimension"); array = new T[getSize(L...)](); }

	void reset() { delete []array; array = 0; }

private:
	sArray<unsigned, d-1>  size;
	T*                     array;

	template <typename... Lengths>
	unsigned getSize(unsigned s, Lengths... L)
	{
		fillInArray(L...);
		for(size_t dd = d - 2; dd > 0; --dd)
			size[dd-1] *= size[dd];
		return s * size[0];
	}
	unsigned getSize(unsigned s) { return s; }

	template <typename... Lengths>
	void fillInArray(unsigned s, Lengths... L) { size[d-2-sizeof...(L)] = s; fillInArray(L...); }
	void fillInArray(unsigned s)               { size[d-2]              = s; }

	template <typename... Lengths>
	unsigned getIdx(unsigned s, Lengths... L)const { return s * size[d-1-sizeof...(L)] + getIdx(L...); }
	unsigned getIdx(unsigned s)              const { return s; }
};

/*========================================================================*/

template <class T, size_t d = 1> using dArray = dArray1<T, d>;
template <class T, size_t d = 1> using  Array = dArray <T, d>;

}

#endif
