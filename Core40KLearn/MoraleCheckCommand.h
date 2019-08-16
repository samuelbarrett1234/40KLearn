#pragma once


#include "IGameCommand.h"


namespace c40kl
{


class MoraleCheckCommand :
	public IGameCommand
{
public:
	MoraleCheckCommand(Position unitPos);

	//Interface functions

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const override;
	virtual bool Equals(const IGameCommand& cmd) const override;
	virtual String ToString() const override;

private:
	Position m_UnitPos;
};


} // namespace c40kl


