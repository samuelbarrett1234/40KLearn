#pragma once


#include <cassert>
#include <exception>
#include <vector>


#ifdef CORE_40KLEARN_EXPORTS
#define C40KL_API __declspec(dllexport)
#else
#define C40KL_API __declspec(dllimport)
#endif


#ifdef _DEBUG

#define C40KL_ASSERT_INVARIANT(expr, msg) assert(expr && msg)
#define C40KL_ASSERT_POSTCONDITION(expr, msg) if(!(expr)) throw new std::runtime_error(msg)
#define C40KL_ASSERT_PRECONDITION(expr, msg) if(!(expr)) throw new std::runtime_error(msg)

#else

#define C40KL_ASSERT_INVARIANT(expr, msg) (void)
#define C40KL_ASSERT_POSTCONDITION(expr, msg) if(!(expr)) throw new std::runtime_error(msg)
#define C40KL_ASSERT_PRECONDITION(expr, msg) if(!(expr)) throw new std::runtime_error(msg)


#endif


namespace c40kl
{


enum class Phase
{
	MOVEMENT,
	SHOOTING,
	CHARGE,
	FIGHT
};


typedef std::pair<int, int> Position;


class C40KL_API PositionArray : public std::vector<Position>
{ };


class C40KL_API IntArray : public std::vector<int>
{ };


class C40KL_API String : public std::string
{
public:
	String() = default;
	String(const String& s) :
		std::string((std::string)s)
	{ }
	String(const std::string& s) :
		std::string(s)
	{ }
};


} // namespace c40kl


