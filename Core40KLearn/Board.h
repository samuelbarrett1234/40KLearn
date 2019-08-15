#pragma once


#include "Utility.h"
#include "Unit.h"


namespace c40kl
{


class C40KL_API BoardState
{
public:
	BoardState(int boardSize);

	bool IsOccupied(int x, int y) const;
	void SetUnitOnSquare(int x, int y, Unit unit, int team);
	const Unit& GetUnitOnSquare(int x, int y) const;
	int GetTeamOnSquare(int x, int y) const;
	PositionArray GetAllUnits(int team) const;
	void ClearSquare(int x, int y);

private:
	int m_Size;

	//All of these arrays should be the same size
	UnitArray m_Units;
	PositionArray m_Positions;
	IntArray m_Teams; // 0 or 1
};


} // namespace c40kl


