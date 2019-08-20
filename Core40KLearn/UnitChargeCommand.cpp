#include "UnitChargeCommand.h"
#include "GameState.h"
#include "GameMechanics.h"
#include "CompositeCommand.h"
#include "OverwatchCommand.h"
#include <sstream>


namespace c40kl
{


//twoDice[i] is the probability of rolling i+2
// as the sum of two dice.
const static float twoDice[] =
{
	1.0f/36.0f, //Dice roll of 2
	2.0f/36.0f,
	3.0f/36.0f,
	4.0f/36.0f,
	5.0f/36.0f,
	6.0f/36.0f, //Dice roll of 7
	5.0f/36.0f,
	4.0f/36.0f,
	3.0f/36.0f,
	2.0f/36.0f,
	1.0f/36.0f //Dice roll of 12
};


void UnitChargeCommand::GetPossibleCommands(const GameState& state, GameCommandArray& outCommands)
{
	//If not in the charge phase, we can't do anything.
	if (state.GetPhase() != Phase::CHARGE)
		return;

	PositionArray possiblePositions;
	Unit stats;

	//Cache these:
	const auto ourTeam = state.GetActingTeam();
	const auto& board = state.GetBoardState();

	//Get all units for this team:
	const auto units = board.GetAllUnits(ourTeam);

	//Get all enemy units:
	const auto enemies = board.GetAllUnits(1 - ourTeam);

	//Consider each unit for charge
	for (const auto& unitPos : units)
	{
		//Get the unit's stats
		stats = board.GetUnitOnSquare(unitPos);

		//Can't charge twice per turn!
		if (!stats.attemptedChargeThisTurn
			//Can't charge if just left combat
			&& !stats.movedOutOfCombatThisTurn
			//Can't charge out of combat
			&& !board.HasAdjacentEnemy(unitPos, ourTeam)
			//And we need a melee weapon
			&& HasStandardMeleeWeapon(stats))
		{
			//Get all of the possible positions to charge to
			// (you can charge a max of 12 inches because you
			// can't get higher than that on the dice).
			possiblePositions = board.GetSquaresInRange(unitPos, 12.0f);

			//For each possible position to charge into...
			for (const auto& targetPos : possiblePositions)
			{
				//Can only charge positions with adjacent enemies
				// and which are not already occupied
				if (!board.IsOccupied(targetPos)
					&& board.HasAdjacentEnemy(targetPos, ourTeam))
				{
					//Construct a composite command, containing all
					// overwatch shooting and then the final charge
					// command:

					GameCommandArray arr;

					//Loop through enemies and if they are adjacent to the charge position,
					// i.e. being charged, and then add an overwatch shoot command:
					for (const auto& enemyPos : enemies)
					{
						//If adjacent...
						if (std::abs(enemyPos.first - targetPos.first) <= 1
							&& std::abs(enemyPos.second - targetPos.second) <= 1)
						{
							const auto& enemyStats = board.GetUnitOnSquare(enemyPos);

							//If satisifes shooting preconditions...
							if (HasStandardRangedWeapon(enemyStats) //If has ranged weapon...
								&& enemyStats.rg_range >= board.GetDistance(unitPos, enemyPos) //... which is in range
								&& !board.HasAdjacentEnemy(enemyPos, 1-ourTeam)) //And if not already tied up in combat...
							{
								//Add overwatch command:
								arr.push_back(std::make_shared<OverwatchCommand>(enemyPos, unitPos));
							}
						}
					}

					//Push charge command with overwatch commands as argument:
					outCommands.push_back(std::make_shared<UnitChargeCommand>(unitPos, targetPos, std::move(arr)));
				}
			}
		}
	}
}


UnitChargeCommand::UnitChargeCommand(Position source, Position target, GameCommandArray&& overwatchCommands) :
	m_Source(source),
	m_Target(target),
	m_Overwatch(overwatchCommands)
{
}


void UnitChargeCommand::Apply(const GameState& startState,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	std::vector<GameState> workingStates;
	std::vector<float> workingDist;
	workingStates.push_back(startState);
	workingDist.push_back(1.0f);

	//First apply overwatch, then apply the charge command
	// to the distribution of results of the overwatch:

	for (auto pCmd : m_Overwatch)
	{
		auto inStates = std::move(workingStates);
		auto inDist = std::move(workingDist);

		C40KL_ASSERT_INVARIANT(workingStates.empty() && workingDist.empty(),
			"Move semantics should clear workingStates and workingDist");

		ApplyCommand(pCmd, inStates, inDist, workingStates, workingDist);
	}

	C40KL_ASSERT_INVARIANT(workingStates.size() == workingDist.size(), "Distribution needs to match.");

	const size_t n = workingStates.size();

	//Apply the charge logic to each resulting overwatch state
	for (size_t i = 0; i < n; i++)
	{
		ApplyChargeCmd(workingStates[i], workingDist[i],
			outStates, outDistribution);
	}
}


bool UnitChargeCommand::Equals(const IGameCommand& cmd) const
{
	if (auto pCmd = dynamic_cast<const UnitChargeCommand*>(&cmd))
	{
		return (m_Source == pCmd->m_Source && m_Target == pCmd->m_Target);
	}
	else return false;
}


String UnitChargeCommand::ToString() const
{
	std::stringstream c;
	c << "charge order from (" << m_Source.first << ',' << m_Source.second
		<< ") to (" << m_Target.first << ',' << m_Target.second << ')';
	return c.str();
}


void UnitChargeCommand::ApplyChargeCmd(const GameState& state, float probOfCurrentState,
	std::vector<GameState>& outStates, std::vector<float>& outDistribution) const
{
	auto board = state.GetBoardState();

	//Note: if the source position is not occupied,
	// then the source unit was killed in overwatch,
	// and we can ignore this command.
	if (!board.IsOccupied(m_Source))
	{
		//Keep state as-is:
		outStates.push_back(state);
		outDistribution.push_back(1.0f*probOfCurrentState);
		return;
	}

	//Check that this action is still valid:
	C40KL_ASSERT_PRECONDITION(
		state.GetPhase() == Phase::CHARGE
		//Don't need to check source is occupied
		//Must be charging to a position which is not occupied
		&& !board.IsOccupied(m_Target)
		//Must be charging to a position with an adjacent enemy
		&& board.HasAdjacentEnemy(m_Target,
			board.GetTeamOnSquare(m_Source))
		//Must have a melee weapon
		&& HasStandardMeleeWeapon(board.GetUnitOnSquare(m_Source))
		//Must have not already attempted to charge this turn
		&& !board.GetUnitOnSquare(m_Source).attemptedChargeThisTurn
		//Must have not moved out of combat this turn
		&& !board.GetUnitOnSquare(m_Source).movedOutOfCombatThisTurn
		, "Charge action preconditions must be satisfied.");

	//Get info:
	auto team = board.GetTeamOnSquare(m_Source);
	auto unitStats = board.GetUnitOnSquare(m_Source);

	//Flag that this unit has moved
	unitStats.attemptedChargeThisTurn = true;

	//There are two possibilities: we make the
	// charge, or we don't.

	//Compute the distance required to make the
	// charge:
	const float distance = board.GetDistance(m_Source, m_Target);

	//The minimum dice roll to succeed:
	const int minDiceRoll = (int)ceil(distance);

	//Sum up the probability that we fail the charge:
	float pFail = 0.0f;
	for (int i = 2; i < minDiceRoll; i++)
	{
		pFail += twoDice[i - 2];
	}
	float pPass = 1.0f - pFail;

	C40KL_ASSERT_INVARIANT(pPass > 0,
		"Should be possible to reach charge destination.");

	//Output the fail state:
	if (pFail > 0)
	{
		outStates.push_back(state);
		outDistribution.push_back(pFail*probOfCurrentState);
	}

	//Compute & output the pass state:

	unitStats.successfulChargeThisTurn = true;

	//Move the unit:
	board.ClearSquare(m_Source);
	board.SetUnitOnSquare(m_Target, unitStats, team);

	//Deterministic action:
	outStates.emplace_back(team, team, Phase::CHARGE, board);
	outDistribution.push_back(pPass*probOfCurrentState);
}


} // namespace c40kl


