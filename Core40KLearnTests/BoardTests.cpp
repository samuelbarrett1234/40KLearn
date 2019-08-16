#include "Test.h"


BOOST_AUTO_TEST_SUITE(BoardTests);


//Test basic square occupation
BOOST_AUTO_TEST_CASE(UnitPlacementTest)
{
	BoardState board(25, 1.0f);

	Position pos(2, 3);

	BOOST_TEST(!board.IsOccupied(pos));
	BOOST_CHECK_THROW(board.GetTeamOnSquare(pos), std::runtime_error);
	BOOST_CHECK_THROW(board.GetUnitOnSquare(pos), std::runtime_error);
	
	Unit u;

	board.SetUnitOnSquare(pos, u, 0);

	BOOST_TEST(board.IsOccupied(pos));
	BOOST_TEST(board.GetTeamOnSquare(pos) == 0);
	BOOST_TEST((board.GetUnitOnSquare(pos) == u));

	BOOST_CHECK_THROW(board.SetUnitOnSquare(pos, u, 2), std::runtime_error);

	board.ClearSquare(pos);

	BOOST_TEST(!board.IsOccupied(pos));
}


BOOST_AUTO_TEST_CASE(GetAllUnitsTest)
{
	BoardState s(25, 1.0f);

	Unit u;

	s.SetUnitOnSquare(Position(0, 0), u, 0);
	s.SetUnitOnSquare(Position(0, 1), u, 0);
	s.SetUnitOnSquare(Position(0, 2), u, 0);
	s.SetUnitOnSquare(Position(1, 0), u, 1);
	s.SetUnitOnSquare(Position(1, 1), u, 1);

	auto team0Units = s.GetAllUnits(0);
	auto team1Units = s.GetAllUnits(1);

	BOOST_REQUIRE(team0Units.size() == 3);
	BOOST_REQUIRE(team1Units.size() == 2);

	BOOST_TEST((Position(0, 0) == team0Units[0]
		|| Position(0, 0) == team0Units[1]
		|| Position(0, 0) == team0Units[2]));

	BOOST_TEST((Position(0, 1) == team0Units[0]
		|| Position(0, 1) == team0Units[1]
		|| Position(0, 1) == team0Units[2]));

	BOOST_TEST((Position(0, 2) == team0Units[0]
		|| Position(0, 2) == team0Units[1]
		|| Position(0, 2) == team0Units[2]));

	BOOST_TEST((Position(1, 0) == team1Units[0]
		|| Position(1, 0) == team1Units[1]));

	BOOST_TEST((Position(1, 1) == team1Units[0]
		|| Position(1, 1) == team1Units[1]));
}


BOOST_AUTO_TEST_CASE(HasAdjacentEnemyTest)
{
	BoardState s(25, 1.0f);

	Unit u;
	Position pos(0, 0);

	BOOST_CHECK_THROW(s.HasAdjacentEnemy(pos, 2), std::runtime_error);

	BOOST_TEST(!s.HasAdjacentEnemy(pos, 0));
	BOOST_TEST(!s.HasAdjacentEnemy(pos, 1));

	s.SetUnitOnSquare(Position(0, 1), u, 0);

	BOOST_TEST(!s.HasAdjacentEnemy(pos, 0));
	BOOST_TEST(s.HasAdjacentEnemy(pos, 1));

	s.SetUnitOnSquare(Position(1, 1), u, 1);

	BOOST_TEST(s.HasAdjacentEnemy(pos, 0));
	BOOST_TEST(s.HasAdjacentEnemy(pos, 1));
}


BOOST_AUTO_TEST_CASE(GetSquaresInRangeCentreTest)
{
	BoardState s(25, 1.0f);

	auto cells = s.GetSquaresInRange(Position(5, 5), 2.0f);
	
	//Helper function
	auto inCells = [&cells](int i, int j) -> bool
	{
		auto iter = std::find(cells.begin(),
			cells.end(), Position(i, j));
		return (iter != cells.end());
	};

	BOOST_TEST(cells.size() == 13);

	//Test the extreme points of the circle
	BOOST_TEST(inCells(5, 5));
	BOOST_TEST(inCells(3, 5));
	BOOST_TEST(inCells(7, 5));
	BOOST_TEST(inCells(5, 3));
	BOOST_TEST(inCells(5, 7));
}


BOOST_AUTO_TEST_CASE(GetSquaresInRangeEdgeTest)
{
	BoardState s(25, 1.0f);

	//Get the three cells in the corner
	auto cells = s.GetSquaresInRange(Position(0, 0), 1.0f);

	BOOST_TEST(cells.size() == 3);

	//We will trust that the coordinates are correct
}


BOOST_AUTO_TEST_CASE(GetSquaresInRangeNonintegerTest)
{
	BoardState s(25, 1.0f);

	//Should not include diagonals because they're distance 1.414...
	auto notQuiteAdjacent = s.GetSquaresInRange(Position(2, 2), 1.0f);

	BOOST_TEST(notQuiteAdjacent.size() == 5);

	//Should now include diagonals!
	auto adjacent = s.GetSquaresInRange(Position(2, 2), 1.45f);

	BOOST_TEST(adjacent.size() == 9);

	//We will trust that the returned values are correct; it
	// is mostly the number of squares to return which is tricky.
}


BOOST_AUTO_TEST_CASE(GetSquaresInRangeScaledTest)
{
	BoardState s(25, 2.0f);
	
	auto cells = s.GetSquaresInRange(Position(5, 5), 2.0f);

	//Helper function
	auto inCells = [&cells](int i, int j) -> bool
	{
		auto iter = std::find(cells.begin(),
			cells.end(), Position(i, j));
		return (iter != cells.end());
	};

	BOOST_TEST(cells.size() == 5);

	//We will trust that the actual positions are correct
}


BOOST_AUTO_TEST_CASE(GetDistanceTest)
{
	BoardState s(25, 2.0f);

	BOOST_TEST(s.GetDistance(Position(0, 1), Position(0, 4)) == 6.0f);
}


BOOST_AUTO_TEST_SUITE_END();


