#pragma once


#include "Utility.h"
#include <memory>


namespace c40kl
{


class GameState;


class C40KL_API IGameCommand
{
public:
	virtual ~IGameCommand() = default;

	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const = 0;

	virtual String ToString() const = 0;

	virtual Position GetSourcePosition() const = 0;

	virtual Position GetTargetPosition() const = 0;
};


typedef std::shared_ptr<IGameCommand> GameCommandPtr;


class C40KL_API GameCommandArray : public std::vector<GameCommandPtr>
{ };


} // namespace c40kl


