from pyapp.game_util import select_randomly
import py40kl


class MCTS:
    """
    root_state : the state for MCTS to begin in.
    tree_policy : the policy to use during the selection stage, where
                  we use value statistics and priors from that node to
                  select the next node to inspect.
    final_policy : the policy which decides which action to take from
                   the root node, after all of the simulations.
    est_strategy : the strategy which (i) computes a value estimate of
                   a particular state and (ii) computes a prior dist.
                   over actions available in a given state.
    """
    def __init__(self, root_state, tree_policy, final_policy, est_strategy):
        self.root = py40kl.MCTSNode.create_root_node(root_state)
        self.tree_policy = tree_policy
        self.final_policy = final_policy
        self.est_strategy = est_strategy
        self.team = root_state.get_acting_team()
        self.maxDepth = 0

    """
    Get the current distribution of actions, as
    informed by all of the simulations performed
    by MCTS.
    Returns [actions], [probabilities].
    """
    def get_distribution(self):
        dist = self.final_policy.get_action_distribution(self.root)
        return self.root.get_actions(), dist

    """
    Get the search tree's estimate for whether or
    not the tree's player is going to win or not.
    Returns the value with respect to the team given
    in the constructor, not the current acting team!
    This is because the value samples added to the
    tree are with respect to 'team', not to the acting
    player.
    """
    def get_value_estimate(self):
        v = self.root.get_value_estimate()
        return v

    """
    Commit to a given action. This RE-ROOTS the MCTS tree
    so as not to waste any of the previous simulations which
    were used to inform the commit decision.
    state : the state which resulted from applying that action
            note that it must be a direct state child of the root.
    """
    def commit(self, state):
        # Max depth of tree will now decrease:
        self.maxDepth -= 1
        children = [self.root.get_state_results(i)
                    for i in range(self.root.get_num_actions())]
        for action_results in children:
            for state_node in action_results:
                if state_node.get_state() == state:
                    self.root = state_node
                    self.root.detach()
                    return None  # Exit function
        raise ValueError("Invalid state given to MCTS commit().")

    """
    Simulate a further n times to improve the current action
    distribution estimate.
    """
    def simulate(self, n):
        print("Simulating", n, "steps...")
        # For each simulation...
        for i in range(n):
            cur_node = self.root

            # Simulate until the end of our MCTS tree:
            while not cur_node.is_leaf() and not cur_node.is_terminal():
                # Compute distribution over actions:
                action_dist = \
                    self.tree_policy.get_action_distribution(cur_node)

                actions = cur_node.get_actions()
                # Select an action according to our tree policy:
                action_idx = select_randomly([i for i in range(len(actions))],
                                             action_dist)

                # Get resulting state distribution:
                state_results = cur_node.get_state_results(action_idx)
                state_dist = cur_node.get_state_result_distribution(action_idx)

                # Select a state (via the random state transition dynamics)
                # and then update the current node
                cur_node = select_randomly(state_results, state_dist)

            # Add value statistic and expand if nonterminal node:
            if cur_node.is_terminal():
                # Note that, in this case, since the state is terminal,
                # this is not an estimate - it is a true value!
                value_estimate = cur_node.get_state().get_game_value(self.team)

                # Add new statistic:
                cur_node.add_value_statistic(value_estimate)

            elif cur_node.is_leaf():
                # This means we need to get actions and prior probabilities
                actions = cur_node.get_actions()

                priors = self.est_strategy.compute_prior_distribution(
                    cur_node.get_state(), actions)

                value_estimate = self.est_strategy.compute_value_estimate(
                    cur_node.get_state())

                # Add new statistic:
                cur_node.add_value_statistic(value_estimate)

                cur_node.expand(priors)

                # Apply an action sampled from the prior:
                action_idx = select_randomly([i for i in range(len(actions))],
                                             priors)

                # Get resulting state distribution:
                state_results = cur_node.get_state_results(action_idx)
                state_dist = cur_node.get_state_result_distribution(action_idx)

                # Select a state (via the random state transition dynamics)
                # and then update the current node
                cur_node = select_randomly(state_results, state_dist)

                # We will then backpropagate this valule below!
                # Determine if we have just gone deeper into the tree:
                depth = cur_node.get_depth()
                if depth > self.maxDepth:
                    self.maxDepth = depth
                    print("MCTS tree deepened to", depth)

    """
    Get the current simulation count from the current tree root.
    NOTE: if you re-root the tree by calling commit(), this number
    might go down (because not all simulations would've taken the
    path that actually did occur).
    """
    def get_num_samples(self):
        return self.root.get_num_value_samples()
