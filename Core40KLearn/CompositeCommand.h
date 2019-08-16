#pragma once


#include "IGameCommand.h"


namespace c40kl
{


//This command simply combines a list of other
// commands and executes them in order, returning
// the appropriate distributions.
class CompositeCommand :
	public IGameCommand
{
public:
	template<typename Iter_t>
	CompositeCommand(Iter_t begin, Iter_t end) :
		m_pCommands(begin, end)
	{ }

	//Interface functions

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const override;
	virtual bool Equals(const IGameCommand& cmd) const override;
	virtual String ToString() const override;

private:
	GameCommandArray m_pCommands;
};


} // namespace c40kl


