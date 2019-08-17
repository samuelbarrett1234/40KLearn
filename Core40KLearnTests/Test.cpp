#include "Test.h"


void stripCommandsNotFor(Position unit, GameCommandArray& cmds)
{
	cmds.erase(std::remove_if(cmds.begin(), cmds.end(), [unit](GameCommandPtr pCmd)
	{
		if (auto pOrderCmd = dynamic_cast<const IUnitOrderCommand*>(pCmd.get()))
		{
			return (pOrderCmd->GetSourcePosition() != unit);
		}
		else return true;
	}), cmds.end());
}


