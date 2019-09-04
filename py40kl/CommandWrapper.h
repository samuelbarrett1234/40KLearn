#pragma once


#include "BoostPython.h"
#include <IGameCommand.h>
#include <GameState.h>
using namespace c40kl;


//Wrapper class needed to wrap shared_ptr type.
//It is unfortunate that we have to wrap this, but as
// of yet, Boost cannot handle vectors of shared-ptrs!
class CommandWrapper
{
public:
	CommandWrapper(const GameCommandPtr& pCmd);

	void Apply(const GameState& state,
		std::vector<GameState>& outStates, std::vector<float>& outDist);
	CommandType GetType() const;
	std::string ToString() const;
	bool operator ==(const CommandWrapper& cmd) const;
	Position GetSourcePosition() const;
	Position GetTargetPosition() const;

private:
	GameCommandPtr m_pCmd;
};


