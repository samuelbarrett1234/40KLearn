#pragma once


#include "IGameCommand.h"


namespace c40kl
{


class UnitChargeCommand :
	public IUnitOrderCommand
{
public:
	/// <summary>
	/// This function gets all possible charge commands
	/// available in the given state and pushes them onto
	/// the outCommands array.
	/// </summary>
	/// <param name="state">The state to issue commands in</param>
	/// <param name="outCommands">The array that contains the command output.</param>
	static void GetPossibleCommands(const GameState& state, GameCommandArray& outCommands);

public:
	UnitChargeCommand(Position source, Position target);

	//Interface functions

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const override;
	virtual bool Equals(const IGameCommand& cmd) const override;
	virtual String ToString() const override;
	virtual inline Position GetSourcePosition() const override
	{
		return m_Source;
	}
	virtual inline Position GetTargetPosition() const override
	{
		return m_Target;
	}

private:
	Position m_Source, m_Target;
};


} // namespace c40kl


