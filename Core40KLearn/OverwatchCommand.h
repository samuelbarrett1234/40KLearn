#pragma once


#include "IGameCommand.h"


namespace c40kl
{


class OverwatchCommand :
	public IUnitOrderCommand
{
public:
	OverwatchCommand(Position source, Position target);

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


