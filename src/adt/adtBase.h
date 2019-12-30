/*========================================================================\
|: [Filename] adtBase.h                                                  :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement some basic type for all ADTs          :|
<------------------------------------------------------------------------*/

#ifndef HEHE_ADTBASE_H
#define HEHE_ADTBASE_H

#include <assert.h>
#include <new>
#include <tuple>
using namespace std;

namespace _54ff
{

template <class T>
class Allocator
{
public:
	T* allocate(size_t n) { return (T*)(operator new(n * sizeof(T))); }
	void deallocate(T* ptr) { operator delete(ptr); }

	template <class... Args>
	void construct(T* ptr, Args&&... args) { new (ptr) T(forward<Args>(args)...); }
	void destroy(T* ptr) { ptr->~T(); }
};

/*================== Some function template alias ==================*/

//Not that robust, expect a more powerful method
#define ALIAS_FUNCTION_TEMPLATE(alias, original)                                     \
template <typename... Args>                                                          \
inline auto alias(Args&&... args) -> decltype(original(std::forward<Args>(args)...)) \
	{ return original(std::forward<Args>(args)...); }

template <class T, class Tuple>
constexpr T MFT(Tuple&& t) { return make_from_tuple<T>(move(t)); }
template <class... Types>
constexpr tuple<Types&&...> FAT(Types&&... args)
	{ return forward_as_tuple(forward<Types>(args)...); }

/*==================         end of alias         ==================*/

template <class F, class S>
struct Pair
{
	Pair(const F& f, const S& s): first(f), second(s) {}
	Pair(F&& f, S&& s): first(move(f)), second(move(s)) {}

	template <class... Args1, class... Args2>
	Pair(tuple<Args1...> args1, tuple<Args2...> args2)
	: first  (MFT<F>(args1))
	, second (MFT<S>(args2)) {}

	F first;
	S second;
};

template <class T>
struct Hasher
{
	size_t operator()(const T& value)const { return size_t(value); }
};

template <class T>
struct EqualTo
{
	bool operator()(const T& v1, const T& v2)const { return v1 == v2; }
};

}

#endif