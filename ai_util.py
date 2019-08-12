import math

"""
Idea: adapt AlphaZero's DCNN/MCTS architecture
- Unlike classical board games, most of our actions have random
    effects, but this is okay because we know the distribution.
- The representation of each piece on the board is more complex
    as we don't want to hard-code all unit types in from the
    beginning (we want the agent to learn what the statistics
    mean).
- We will use a DCNN to obtain a heuristic state value (i.e. the
    DCNN will be a value function approximator).
- We will perform one MCTS search throughout a single turn of the
    agent. This is because, during our turn, the opponent has no
    actions to make, so the MCTS results can persist between actions
    in a single turn.
- MCTS gets a better estimate of value, guided by DCNN heuristics,
    which then is used to improve the DCNN and select the action to
    take. This is repeated until the end of the episode. I.e. MCTS
    is our policy!
- We should use experience replay (!) and fixed Q-targets (?)
- When MCTS evaluates a heuristic using the DCNN it should randomly
    apply one of 8 symmetries to the board first, to prevent bias.
- No discount factor; reward of 1, 0 or -1 given at the end of the game.
- MCTS simulates both teams, even when it comes to fully evaluating
    the other team's actions! This is done according to its policy.
- IDEA: could we pre-train the DCNN on example games first? This could
    be achieved by recording the intermediate states of a game and
    labelling the data with the result at the end of the game.
"""


class MCTSNode:
    """
    Initialise the node.
    parent : the parent of this node (note that this node will insert
             itself into the children array of parent if it is not None)
    action : the action which obtained the state of this node from the
             parent
    state : the current state of the node
    myWeight : the probability of the given action resulting in the given
               state.
    """
    def __init__(self, parent, action, state, myWeight):
        self.parent = parent
        self.children = []
        if parent is not None:
            self.parent.children.append(self)
        self.action = action
        self.state = state
        self.values = []
        self.weights = []
        self.numVisits = []
        self.myWeight = myWeight
    
    """
    Compute the estimated value of this node, computed from the statistics
    it has received via simulation.
    """
    def getValue(self):
        assert(self.numVisits > 0)
        #UCB1
        return sum(self.values) / sum(self.weights) + (math.log(self.parent.numVisits) / self.numVisits) ** 0.5
    
    """
    After computing a simulation from this state or a child state, calling
    this with the relevant value and weight is necessary to record it in
    this node. The weights could all be set to 1 if we have even weighting,
    but if the result of an action is random, the dynamics of that action
    should be used as the weighting, if they are known.
    """
    def addValue(self, value, weight):
        self.values.append(value * weight)
        self.weights.append(weight)
        self.numVisits += 1


class MCTS:
    def __init__(self, rootState):
        self.root = MCTSNode(None, None, rootState, 1.0)
        
    """
    Simulate from the given node and backpropagate the results.
    """
    def simulateFrom(self, node):
        val = self.getSimulatedValue(node.state)
        weightAccumulation = 1.0
        while node is not None:
            node.addValue(val, weightAccumulation)
            weightAccumulation *= node.myWeight
            node = node.parent
        
    """
    Interface function
    This should return the value of a simulated game from this state.
    This can be done by (i) random moves until termination, (ii)
    calculated moves until termination, (iii) cutting off simulation
    after a few moves and letting a value function approximator estimate
    the value of the rest of the simulation.
    """
    def getSimulatedValue(self, state):
        pass
        
        