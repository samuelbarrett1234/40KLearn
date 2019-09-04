#include "CommandWrapper.h"


CommandWrapper::CommandWrapper(const GameCommandPtr& pCmd) :
	m_pCmd(pCmd)
{ }


void CommandWrapper::Apply(const GameState& state,
	std::vector<GameState>& outStates, std::vector<float>& outDist)
{
	m_pCmd->Apply(state, outStates, outDist);
}


CommandType CommandWrapper::GetType() const
{
	return m_pCmd->GetType();
}


std::string CommandWrapper::ToString() const
{
	return m_pCmd->ToString();
}


bool CommandWrapper::operator ==(const CommandWrapper& cmd) const
{
	return m_pCmd->Equals(*cmd.m_pCmd);
}


Position CommandWrapper::GetSourcePosition() const
{
	if (auto pCmd = dynamic_cast<const IUnitOrderCommand*>(m_pCmd.get()))
	{
		return pCmd->GetSourcePosition();
	}
	else throw std::runtime_error("Command object was not a unit order command.");
}


Position CommandWrapper::GetTargetPosition() const
{
	if (auto pCmd = dynamic_cast<const IUnitOrderCommand*>(m_pCmd.get()))
	{
		return pCmd->GetTargetPosition();
	}
	else throw std::runtime_error("Command object was not a unit order command.");
}


