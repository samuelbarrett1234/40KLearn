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
	BoardState(int boardSize);

	/// <summary>
	/// Check if a unit lies on the given grid cell.
	/// Precondition: x and y are coordinates in a valid range.
	/// </summary>
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	/// <returns>True if there is a unit (on either team) on this square, false if not.</returns>
	bool IsOccupied(int x, int y) const;

	/// <summary>
	/// Place a unit on the given square, overriding any
	/// previous placements on this square.
	/// </summary>
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	/// <param name="unit">The statistics of the unit to place.</param>
	/// <param name="team">The team of the unit (must be 0 or 1).</param>
	void SetUnitOnSquare(int x, int y, Unit unit, int team);

	/// <summary>
	/// Return the statistics of the unit on the given square.
	/// Precondition: IsOccupied(x,y).
	/// </summary>
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	/// <returns>A reference to the unit statistics (feel free to copy).</returns>
	const Unit& GetUnitOnSquare(int x, int y) const;

	/// <summary>
	/// Return the team of the unit on the given square.
	/// Precondition: IsOccupied(x,y).
	/// </summary>
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	/// <returns>0 or 1, depending on the team.</returns>
	int GetTeamOnSquare(int x, int y) const;

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
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	void ClearSquare(int x, int y);

	/// <summary>
	/// Check if any of the squares adjacent to (but not including) (x,y)
	/// contain a unit whose team differs from 'team'.
	/// </summary>
	/// <param name="x">Zero-indexed x coordinate of cell.</param>
	/// <param name="y">Zero-indexed y coordinate of cell.</param>
	/// <param name="team">0 or 1; the team you want to check against.</param>
	/// <returns>True if and only if the above condition holds.</returns>
	bool HasAdjacentEnemy(int x, int y, int team) const;

private:
	int m_Size;

	//All of these arrays should be the same size
	UnitArray m_Units;
	PositionArray m_Positions;
	IntArray m_Teams; // 0 or 1
};


} // namespace c40kl


