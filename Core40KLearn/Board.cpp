#include "Board.h"


namespace c40kl
{


BoardState::BoardState(int boardSize) :
	m_Size(boardSize)
{
	C40KL_ASSERT_PRECONDITION(boardSize > 0, "Board size must be strictly positive.");
}


bool BoardState::IsOccupied(int x, int y) const
{
	C40KL_ASSERT_PRECONDITION(x >= 0 && y >= 0 && x < m_Size && y < m_Size,
		"Coordinates must be valid.");

	Position pos(x, y);
	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);
	return (iter != m_Positions.end());
}


void BoardState::SetUnitOnSquare(int x, int y, Unit unit, int team)
{
	C40KL_ASSERT_PRECONDITION(x >= 0 && y >= 0 && x < m_Size && y < m_Size,
		"Coordinates must be valid.");
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Team must be 0 or 1.");

	Position pos(x, y);
	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);
	
	if (iter == m_Positions.end())
	{
		//Previously unoccupied position
		m_Units.push_back(unit);
		m_Positions.push_back(pos);
		m_Teams.push_back(team);
	}
	else
	{
		//Override existing info
		size_t i = std::distance(m_Positions.begin(), iter);
		m_Teams[i] = team;
		m_Units[i] = unit;
	}
}


const Unit& BoardState::GetUnitOnSquare(int x, int y) const
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(x, y), "Must be an occupied square.");

	Position pos(x, y);
	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);
	
	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	size_t i = std::distance(m_Positions.begin(), iter);

	return m_Units[i];
}


int BoardState::GetTeamOnSquare(int x, int y) const
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(x, y), "Must be an occupied square.");

	Position pos(x, y);
	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);

	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	size_t i = std::distance(m_Positions.begin(), iter);

	return m_Teams[i];
}


PositionArray BoardState::GetAllUnits(int team) const
{
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Team must be 0 or 1.");
	C40KL_ASSERT_INVARIANT(m_Positions.size() == m_Teams.size(),
		"Position/team/unit arrays must be same size.");

	PositionArray arr;

	for (size_t i = 0; i < m_Positions.size(); i++)
	{
		if (m_Teams[i] == team)
		{
			arr.push_back(m_Positions[i]);
		}
	}

	return arr;
}


void BoardState::ClearSquare(int x, int y)
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(x, y), "Must be an occupied square.");

	Position pos(x, y);
	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);

	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	size_t i = std::distance(m_Positions.begin(), iter);

	//Erase:
	m_Positions.erase(m_Positions.begin() + i);
	m_Teams.erase(m_Teams.begin() + i);
	m_Units.erase(m_Units.begin() + i);
}


} // namespace c40kl


