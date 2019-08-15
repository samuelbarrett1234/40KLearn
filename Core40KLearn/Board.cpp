#include "Board.h"
#include <algorithm>
#include <cmath>


namespace c40kl
{


BoardState::BoardState(int boardSize, float scale) :
	m_Size(boardSize),
	m_Scale(scale)
{
	C40KL_ASSERT_PRECONDITION(boardSize > 0, "Board size must be strictly positive.");
	C40KL_ASSERT_PRECONDITION(scale > 0, "Scale must be strictly positive.");
}


bool BoardState::IsOccupied(Position pos) const
{
	C40KL_ASSERT_PRECONDITION(pos.first >= 0 && pos.second >= 0 && pos.first < m_Size && pos.second < m_Size,
		"Coordinates must be valid.");

	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);
	return (iter != m_Positions.end());
}


void BoardState::SetUnitOnSquare(Position pos, Unit unit, int team)
{
	C40KL_ASSERT_PRECONDITION(pos.first >= 0 && pos.second >= 0 && pos.first < m_Size && pos.second < m_Size,
		"Coordinates must be valid.");
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Team must be 0 or 1.");

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


const Unit& BoardState::GetUnitOnSquare(Position pos) const
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(pos), "Must be an occupied square.");

	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);
	
	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	size_t i = std::distance(m_Positions.begin(), iter);

	return m_Units[i];
}


int BoardState::GetTeamOnSquare(Position pos) const
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(pos), "Must be an occupied square.");

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


void BoardState::ClearSquare(Position pos)
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(pos), "Must be an occupied square.");

	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);

	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	size_t i = std::distance(m_Positions.begin(), iter);

	//Erase:
	m_Positions.erase(m_Positions.begin() + i);
	m_Teams.erase(m_Teams.begin() + i);
	m_Units.erase(m_Units.begin() + i);
}


bool BoardState::HasAdjacentEnemy(Position pos, int team) const
{
	for (int i = pos.first - 1; i <= pos.first + 1; i++)
	{
		for (int j = pos.second - 1; j <= pos.second + 1; j++)
		{
			if (i >= 0 && i < m_Size && j >= 0 && j < m_Size)
			{
				if (IsOccupied(Position(i,j)) && GetTeamOnSquare(Position(i,j)) != team)
				{
					return true;
				}
			}
		}
	}
	return false;
}


PositionArray BoardState::GetSquaresInRange(Position centre, float radius) const
{
	const int intRad = (int)(radius / m_Scale);
	const int intRadSq = intRad * intRad;

	//Compute 'loose' rectangle
	const int left = std::max(0, centre.first - intRad);
	const int right = std::min(m_Size - 1, centre.first + intRad);
	const int top = std::max(0, centre.second - intRad);
	const int bottom = std::min(m_Size - 1, centre.second + intRad);

	PositionArray result;

	//There will be just over pi*r^2 cells, so reserve as much
	// to prevent extra allocations
	result.reserve(4 * intRadSq);

	//Loop through loose rectangle, push_back
	// elements which are within the circle.
	for (size_t i = left; i <= right; i++)
	{
		for (size_t j = top; j <= bottom; j++)
		{
			const int dx = centre.first - i, dy = centre.second - j;
			if (dx*dx + dy * dy <= intRadSq)
			{
				result.push_back(Position(i, j));
			}
		}
	}

	return result;
}


float BoardState::GetDistance(Position a, Position b) const
{
	const auto dx = a.first - b.first;
	const auto dy = a.second - b.second;
	return m_Scale * std::sqrt(dx*dx+dy*dy);
}


} // namespace c40kl


