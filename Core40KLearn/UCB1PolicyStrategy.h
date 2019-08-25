#pragma once


#include "IPolicyStrategy.h"


namespace c40kl
{


/// <summary>
/// UCB1 is a good policy strategy for automatically balancing
/// exploration and exploitation, based on the number of times
/// the set of actions has been tried. In MCTS, this policy
/// is used as the tree policy - this means it should be used
/// to pick which next action to evaluate in the MCTS tree,
/// and hence *only operates on non-leaf nodes*!
/// </summary>
class C40KL_API UCB1PolicyStrategy :
	public IPolicyStrategy
{
public:
	/// <summary>
	/// Create a new UCB1PolicyStrategy.
	/// exploratoryParam : a value which affects how much this policy
	///                    explores; theoretically equal to sqrt(2) but
	///                    can be chosen empirically. Higher => more
	///                    exploration! Must be >= 0.
	/// rootTeam : this is the team with to whom the value estimates
	///            are attributed. This means that, if the given node
	///            is choosing between actions for this team, it will
	///            pick the best one, and if it is the opposing team
	///            making the decisions, it will pick the worst action
	///            for the rootTeam.
	/// </summary>
	UCB1PolicyStrategy(float exploratoryParam, int rootTeam);


	virtual std::vector<float> GetActionDistribution(const MCTSNode& node) const override;


private:
	float m_ExploratoryParam;
	int m_Team;
};


} // namespace c40kl


