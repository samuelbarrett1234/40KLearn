#pragma once


#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>


#include <Board.h>
#include <GameState.h>


#ifdef C40KL_CHECK_PRE_POST_CONDITIONS

#define C40KL_CHECK_PRE_POST_EXCEPTION(expr) BOOST_CHECK_THROW(expr, std::runtime_error)

#else

#define C40KL_CHECK_PRE_POST_EXCEPTION(expr) ((void)0)

#endif


using namespace c40kl;


/// <summary>
/// Remove all commands in the array which are not directly
/// ordering 'unit'.
/// </summary>
void stripCommandsNotFor(Position unit, GameCommandArray& cmds);


