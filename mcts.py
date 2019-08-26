from game_util import selectRandomly
import py40kl


class MCTS:
    """
    root_state : the state for MCTS to begin in.
    tree_policy : the policy which decides the distribution of actions from a leaf node
    final_policy : the policy which decides which action to take from the root node, after many simulations
    sim_strategy : the strategy which decides the value of a state by simulation or otherwise.
    """
    def __init__(self, root_state, tree_policy, final_policy, sim_strategy):
        self.root = py40kl.MCTSNode.create_root_node(root_state)
        self.tree_policy = tree_policy
        self.final_policy = final_policy
        self.sim_strategy = sim_strategy
        self.team = root_state.get_acting_team()
        self.maxDepth = 0
        
    """
    Get the current distribution of actions, as
    informed by all of the simulations performed
    by MCTS.
    Returns [actions], [probabilities].
    """
    def get_distribution(self):
        values = self.root.get_action_value_estimates()
        visit_counts = self.root.get_action_visit_counts()
        priors = self.root.get_action_prior_distribution()
        dist = self.final_policy.get_action_distribution(values, visit_counts, priors)
        return self.root.get_actions(),dist
        
    """
    Commit to a given action. This RE-ROOTS the MCTS tree
    so as not to waste any of the previous simulations which
    were used to inform the commit decision.
    state : the state which resulted from applying that action
            note that it must be a direct state child of the root.
    """
    def commit(self, state):
        children = [self.root.get_state_results(i) for i in range(self.root.get_num_actions())]
        for action_results in children:
            for state_node in action_results:
                if state_node.get_state() == state:
                    self.root = state_node
                    self.root.detach()
                    return None #Exit function
        raise ValueError("Invalid state given to MCTS commit().")
        
    """
    Simulate a further n times to improve the current action
    distribution estimate.
    """
    def simulate(self, n):
        #For each simulation...
        for i in range(n):
            cur_node = self.root
            
            #Simulate until the end of our MCTS tree:
            while not cur_node.is_leaf() and not cur_node.is_terminal():
                #Obtain information about potential actions:
                values = cur_node.get_action_value_estimates()
                visit_counts = cur_node.get_action_visit_counts()
                priors = cur_node.get_action_prior_distribution()
                
                #Compute distribution over actions:
                action_dist = self.tree_policy.get_action_distribution(values, visit_counts, priors)
                
                actions = cur_node.get_actions()
                #Select an action according to our tree policy:
                action_idx = selectRandomly([i for i in range(len(actions))], action_dist)
                
                #Get resulting state distribution:
                state_results = cur_node.get_state_results(action_idx)
                state_dist = cur_node.get_state_result_distribution(action_idx)
                
                #Select a state (via the random state transition dynamics)
                # and then update the current node
                cur_node = selectRandomly(state_results, state_dist)
                
            #Compute value estimate of this state
            value_estimate = None
            if cur_node.is_terminal():
                #Note that, in this case, since the state is terminal,
                # this is not an estimate - it is a true value!
                value_estimate = cur_node.get_state().get_game_value(self.team)
            elif cur_node.isLeaf():
                #Expand the leaf node BEFORE doing a simulation
                #This means we need to get actions and prior probabilities
                actions = cur_node.get_actions()
                priors = self.sim_strategy.compute_prior_distribution(cur_node.getState(), actions)
                cur_node.expand(priors)
                
                #Apply an action sampled from the prior:
                action_idx = selectRandomly([i for i in range(len(actions))],priors)
                                
                #Get resulting state distribution:
                state_results = cur_node.get_state_results(action_idx)
                state_dist = cur_node.get_state_result_distribution(action_idx)
                
                #Select a state (via the random state transition dynamics)
                # and then update the current node
                cur_node = selectRandomly(state_results, state_dist)
                
                #Simulate to compute its value:
                value_estimate = self.sim_strategy.compute_value_estimate(cur_node.get_state())
                
                #We will then backpropagate this valule below!
                #Determine if we have just gone deeper into the tree:
                depth = cur_node.get_depth()
                if depth > self.maxDepth:
                    print("MCTS tree deepened to", depth)
                    self.maxDepth = depth
                
            #Add new statistic:
            cur_node.add_value_statistic(value_estimate)
            
            #Done!
            print("Simulation",i,"completed.")
        
    """
    Get the current simulation count from the current tree root.
    NOTE: if you re-root the tree by calling commit(), this number
    might go down (because not all simulations would've taken the
    path that actually did occur).
    """
    def get_num_samples(self):
        return self.root.get_num_value_samples()
        