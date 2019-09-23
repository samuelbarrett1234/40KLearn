#include "Board.h"
#include <algorithm>
#include <cmath>
#include <sstream>


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

		const size_t i = std::distance(m_Positions.begin(), iter);

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

	const size_t i = std::distance(m_Positions.begin(), iter);

	return m_Units[i];
}


int BoardState::GetTeamOnSquare(Position pos) const
{
	//Automatically checks x,y are valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(pos), "Must be an occupied square.");

	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);

	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	const size_t i = std::distance(m_Positions.begin(), iter);

	return m_Teams[i];
}


PositionArray BoardState::GetAllUnits(int team) const
{
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Team must be 0 or 1.");
	C40KL_ASSERT_INVARIANT(m_Positions.size() == m_Teams.size(),
		"Position/team/unit arrays must be same size.");

	PositionArray arr;

	//Reserve for worst case to prevent lots of allocations
	arr.reserve(m_Positions.size());

	for (size_t i = 0; i < m_Positions.size(); i++)
	{
		if (m_Teams[i] == team)
		{
			arr.push_back(m_Positions[i]);
		}
	}

	return arr;
}


UnitArray BoardState::GetAllUnitStats(int team) const
{
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Team must be 0 or 1.");
	C40KL_ASSERT_INVARIANT(m_Units.size() == m_Teams.size(),
		"Position/team/unit arrays must be same size.");

	UnitArray arr;

	//Reserve for worst case to prevent lots of allocations
	arr.reserve(m_Units.size());

	for (size_t i = 0; i < m_Units.size(); i++)
	{
		if (m_Teams[i] == team)
		{
			arr.push_back(m_Units[i]);
		}
	}

	return arr;
}


void BoardState::ClearSquare(Position pos)
{
	//Automatically checks pos is valid
	C40KL_ASSERT_PRECONDITION(IsOccupied(pos), "Must be an occupied square.");

	auto iter = std::find(m_Positions.begin(), m_Positions.end(), pos);

	C40KL_ASSERT_INVARIANT(iter != m_Positions.end(), "Must be able to find position.");

	const size_t i = std::distance(m_Positions.begin(), iter);

	//Erase using 'swap and pop':

	std::swap(m_Positions[i], m_Positions.back());
	m_Positions.pop_back();

	std::swap(m_Teams[i], m_Teams.back());
	m_Teams.pop_back();

	std::swap(m_Units[i], m_Units.back());
	m_Units.pop_back();
}


bool BoardState::HasAdjacentEnemy(Position pos, int team) const
{
	C40KL_ASSERT_PRECONDITION(team == 0 || team == 1, "Invalid team value.");
	for (size_t i = 0; i < m_Positions.size(); i++)
	{
		if (m_Teams[i] != team &&
			std::abs(m_Positions[i].first - pos.first) <= 1
			&& std::abs(m_Positions[i].second - pos.second) <= 1
			&& m_Positions[i] != pos)
		{
			return true;
		}
	}
	return false;
}


PositionArray BoardState::GetSquaresInRange(Position centre, float radius) const
{
	const int intRad = (int)ceil(radius / m_Scale);
	const int intRadSq = (int)floor(radius * radius / m_Scale / m_Scale);

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
	for (int i = left; i <= right; i++)
	{
		for (int j = top; j <= bottom; j++)
		{
			C40KL_ASSERT_INVARIANT(i >= 0 && j >= 0 && i < m_Size && j < m_Size,
				"Coordinates should be valid.");

			const int dx = centre.first - i, dy = centre.second - j;
			if (dx * dx + dy * dy <= intRadSq)
			{
				result.emplace_back(i, j);
			}
		}
	}

	return result;
}


float BoardState::GetDistance(Position a, Position b) const
{
	const auto dx = a.first - b.first;
	const auto dy = a.second - b.second;
	return m_Scale * std::sqrtf(static_cast<float>(dx * dx + dy * dy));
}


std::pair<size_t, size_t> BoardState::GetUnitCounts() const
{
	std::pair<size_t, size_t> result = std::make_pair(0U, 0U);

	for (auto team : m_Teams)
	{
		switch (team)
		{
		case 0:
			result.first++;
			break;
		case 1:
			result.second++;
			break;
		}
	}

	return result;
}


std::string BoardState::ToString() const
{
	std::stringstream m;

	m << "Board State ( size = " << m_Size
		<< ", scale = " << m_Scale << ", units = [  ";

	for (size_t i = 0; i < m_Positions.size(); i++)
	{
		m << '"' << m_Units[i].name << '"'
			<< " at (" << m_Positions[i].first << ','
			<< m_Positions[i].second << ")  ";
	}

	m << "] )";

	return m.str();
}


} // namespace c40kl


