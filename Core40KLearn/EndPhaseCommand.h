#pragma once


#include "IGameCommand.h"


namespace c40kl
{


class EndPhaseCommand :
	public IGameCommand
{
public:
	/// <summary>
	/// This function gets all possible end phase commands
	/// available in the given state and pushes them onto
	/// the outCommands array. Note that due to the nature
	/// of this command, there is always exactly one such
	/// command available to the player.
	/// </summary>
	/// <param name="state">The state to issue commands in</param>
	/// <param name="outCommands">The array that contains the command output.</param>
	static void GetPossibleCommands(const GameState& state, GameCommandArray& outCommands);

public:
	//Interface functions

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const override;
	virtual bool Equals(const IGameCommand& cmd) const override;
	virtual String ToString() const override;
	virtual inline CommandType GetType() const override
	{
		return CommandType::END_PHASE;
	}
};


} // namespace c40kl


