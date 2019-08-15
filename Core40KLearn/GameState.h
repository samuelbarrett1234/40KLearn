#pragma once


#include "Utility.h"
#include "Board.h"


namespace c40kl
{


class C40KL_API GameState
{
public:
	GameState();

private:
	BoardState m_Board;
};


} // namespace c40kl


