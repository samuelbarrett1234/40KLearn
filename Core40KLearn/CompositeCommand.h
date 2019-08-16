#pragma once


#include "IGameCommand.h"


namespace c40kl
{


/// <summary>
/// This command simply combines a list of other
/// commands and executes them in order, returning
/// the appropriate distributions.
/// Note: you must also provide it with a command
/// type.
/// </summary>
class CompositeCommand :
	public IGameCommand
{
public:
	template<typename Iter_t>
	CompositeCommand(Iter_t begin, Iter_t end, CommandType myType) :
		m_pCommands(begin, end),
		m_Type(myType)
	{ }

	//Interface functions

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const override;
	virtual bool Equals(const IGameCommand& cmd) const override;
	virtual String ToString() const override;
	virtual inline CommandType GetType() const override
	{
		return m_Type;
	}

private:
	GameCommandArray m_pCommands;
	CommandType m_Type;
};


} // namespace c40kl


