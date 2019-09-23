#pragma once


#include <cassert>
#include <exception>
#include <vector>
#include <string>


//We are using the same compiler version for all projects
// so disable the warning about using STL variables in
// exported classes.
#pragma warning(disable:4251)


#ifdef CORE_40KLEARN_EXPORTS
#define C40KL_API __declspec(dllexport)
#else
#define C40KL_API __declspec(dllimport)
#endif


#ifdef _DEBUG

#define C40KL_ASSERT_INVARIANT(expr, msg) assert(expr && msg)
#define C40KL_ASSERT_POSTCONDITION(expr, msg) if(!(expr)) throw std::runtime_error(msg)
#define C40KL_ASSERT_PRECONDITION(expr, msg) if(!(expr)) throw std::runtime_error(msg)

#else

#define C40KL_ASSERT_INVARIANT(expr, msg) ((void)0)
#define C40KL_ASSERT_POSTCONDITION(expr, msg) if(!(expr)) throw std::runtime_error(msg)
#define C40KL_ASSERT_PRECONDITION(expr, msg) if(!(expr)) throw std::runtime_error(msg)


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


typedef std::vector<Position> PositionArray;
typedef std::vector<int> IntArray;
typedef std::string String;


} // namespace c40kl


