#pragma once


#include "Utility.h"
#include "Unit.h"


namespace c40kl
{


/// <summary>
/// The board state represents units and their teams
/// on a fixed-sized square grid. This is not an
/// immutable data structure and can be modified.
/// </summary>
class C40KL_API BoardState
{
public:
	/// <summary>
	/// Initialise the board state with a particular size.
	/// </summary>
	/// <param name="boardSize">The dimensions of the board (must be greater than 0)</param>
	/// <param name="scale">The length of a cell.</param>
	BoardState(int boardSize, float scale);

	/// <summary>
	/// Check if a unit lies on the given grid cell.
	/// Precondition: x and y are coordinates in a valid range.
	/// </summary>
	/// <param name="pos">The position to check at</param>
	/// <returns>True if there is a unit (on either team) on this square, false if not.</returns>
	bool IsOccupied(Position pos) const;

	/// <summary>
	/// Place a unit on the given square, overriding any
	/// previous placements on this square.
	/// </summary>
	/// <param name="pos">The position to place the unit at</param>
	/// <param name="unit">The statistics of the unit to place.</param>
	/// <param name="team">The team of the unit (must be 0 or 1).</param>
	void SetUnitOnSquare(Position pos, Unit unit, int team);

	/// <summary>
	/// Return the statistics of the unit on the given square.
	/// Precondition: IsOccupied(x,y).
	/// </summary>
	/// <param name="pos">The position to get the unit at</param>
	/// <returns>A reference to the unit statistics (feel free to copy).</returns>
	const Unit& GetUnitOnSquare(Position pos) const;

	/// <summary>
	/// Return the team of the unit on the given square.
	/// Precondition: IsOccupied(x,y).
	/// </summary>
	/// <param name="pos">The position to get the team at</param>
	/// <returns>0 or 1, depending on the team.</returns>
	int GetTeamOnSquare(Position pos) const;

	/// <summary>
	/// Return an array containing all of the units belonging
	/// to a particular team.
	/// </summary>
	/// <param name="team">0 or 1; the team of the units you want to get.</param>
	/// <returns>A new array of unit positions. You may look up the statistics
	/// by querying the positions.</returns>
	PositionArray GetAllUnits(int team) const;

	/// <summary>
	/// Remove the unit from the given square.
	/// Precondition: IsOccupied(x,y).
	/// Postcondition: !IsOccupied(x,y).
	/// </summary>
	/// <param name="pos">The position to clear</param>
	void ClearSquare(Position pos);

	/// <summary>
	/// Check if any of the squares adjacent to (but not including) (x,y)
	/// contain a unit whose team differs from 'team'.
	/// </summary>
	/// <param name="pos">The position to check at</param>
	/// <param name="team">0 or 1; the team you want to check against.</param>
	/// <returns>True if and only if the above condition holds.</returns>
	bool HasAdjacentEnemy(Position pos, int team) const;

	/// <summary>
	/// Return all squares which are (i) valid positions
	/// on this board, and (ii) are at most a distance
	/// "radius" from the point "centre" in "real world"
	/// scale, not grid cell scale.
	/// Precondition: the centre is a valid point.
	/// </summary>
	/// <param name="centre">The centre coordinate.</param>
	/// <param name="radius">The maximum radius in "real world" scale, not grid cell scale..</param>
	/// <returns>An array of all positions satisfying (i) and (ii).</return>
	PositionArray GetSquaresInRange(Position centre, float radius) const;

	/// <summary>
	/// Get the "real world" distance between these two points.
	/// </summary>
	/// <param name="a">The first point</param>
	/// <param name="b">The second point</param>
	/// <returns>The real world distance between a and b</returns>
	float GetDistance(Position a, Position b) const;


	inline bool operator == (const BoardState& other) const
	{
		return (m_Size == other.m_Size && m_Scale == other.m_Scale
			&& m_Units == other.m_Units && m_Positions == other.m_Positions
			&& m_Teams == other.m_Teams);
	}

private:
	int m_Size;
	float m_Scale;

	//All of these arrays should be the same size
	UnitArray m_Units;
	PositionArray m_Positions;
	IntArray m_Teams; // 0 or 1
};


} // namespace c40kl


