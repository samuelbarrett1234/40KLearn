#pragma once


#include "Utility.h"
#include "Board.h"
#include "GameCommands.h"


namespace c40kl
{


class C40KL_API GameState
{
public:
	GameState();

	int GetTeam() const;
	GameCommandArray GetCommands() const;
	bool IsFinished() const;
	int GetWinner() const;
	Phase GetPhase() const;

private:
	BoardState m_Board;
};


} // namespace c40kl


