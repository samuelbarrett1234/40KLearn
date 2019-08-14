"""
An action node is a node in the MCTS tree which
contains information about the current action,
the distribution of states it could potentially
lead to, our prior confidence in this action, and
it also keeps track of its estimated value by
looking at the estimated value of its state node children.
"""
class ActionNode:
    def __init__(self, parentStateNode, actionCmd, priorProbability):
        self.parent = parentStateNode
        self.action = actionCmd
        self.prior = priorProbability
        #Immediately set up child state nodes, and their probabilities:
        states, self.probs = actionCmd.apply(parentStateNode.getState())
        self.stateNodeChildren = [StateNode(state, self, prob) for state in states]
        #We will cache our current value estimate of this action, derived
        # from the child states' estimates.
        self.currentEstimate = None
        self.sampleCount = 0
        
    """
    Update the estimate of this action's value. This should
    be called whenever any of its child nodes have had their
    value changed, say, by adding a new sample.
    """
    def updateEstimate(self):
        #Compute average value, weighted by the probability
        # distribution: self.probs
        weights = [child.getSampleCount()*w for child,w in zip(self.stateNodeChildren, self.probs)]
        values = [child.getValueEstimate() for child in self.stateNodeChildren]
        weightSum = sum(weights)
        assert(weightSum > 0.0)
        self.currentEstimate = sum([v*nw for v,nw in zip(values,weights)]) / weightSum
        self.sampleCount = sum([child.getSampleCount() for child in self.stateNodeChildren])
        
    def getValueEstimate(self):
        assert(self.currentEstimate is not None)
        return self.currentEstimate
        
    def getPrior(self):
        return self.prior
        
    def getSampleCount(self):
        return self.sampleCount
        
    def getActionCommand(self):
        return self.action
        
    def getChildNodes(self):
        return self.stateNodeChildren
        
    def getChildNodeDistribution(self):
        return self.probs
        
    def getParent(self):
        return self.parent
        
        
        
"""
A state node represents a potential state in the MCTS
tree, and has ActionNode children and parent. Contains
all of the samples gathered by simulating from this node
and/or nodes further down the tree. Can be terminal and/or
a leaf node and/or a root node.
"""
class StateNode:
    def __init__(self, state, actionParent, transitionProbability):
        self.state = state
        self.parent = actionParent
        self.transitionProbability = transitionProbability
        self.actionChildren = [] #list of action NODES
        self.numSamples = 0 #The number of samples that meanValue consists of
        self.weightSum = 0.0 #The sum of the weights of the samples in meanValue
        self.meanValue = 0.0 #The weighted mean value from this state so far
        
    """
    If this node is a nonterminal leaf node in the MCTS tree,
    calling this will expand it with all possible child actions.
    """
    def expand(self, actions, probs):
        assert(self.isLeaf() and not self.isTerminal())
        self.actionChildren = [ActionNode(self, action, p) for action,p in zip(actions,probs)]
        
    """
    Determine if this state is terminal (which means no more
    actions can be performed).
    """
    def isTerminal(self):
        return self.state.finished()
        
    """
    Determine if this state is a leaf state (which means that
    its actions haven't been examined yet).
    """
    def isLeaf(self):
        return (len(self.actionChildren) == 0)
        
    """
    Determine if this node is the root of the MCTS tree.
    """
    def isRoot(self):
        return (self.parent is None)
        
    """
    Add a value statistic to this node (this value should be
    an estimate of the expected returns from this node. This
    may be derived from a simulation directly from this state,
    or from a future state.)
    Here, the weight is the probability of the child state's (from
    which the value is being derived) probability of occurring
    given the actions taken to get there.
    """
    def addStatistic(self, value, weight):
        #Incrementally update the mean
        self.meanValue = (self.weightSum * self.meanValue + value * weight) / (self.weightSum + weight)
        self.weightSum += weight
        self.numSamples += 1
        
    """
    Get the number of value estimate samples in this state.
    """
    def getSampleCount(self):
        return self.numSamples
        
    """
    Get the value estimate from this state by considering
    samples given so far.
    """
    def getValueEstimate(self):
        assert(self.numSamples > 0)
        return self.meanValue
        
    """
    Get the action node which lead to this state, or None
    if this is the root node.
    """
    def getParentAction(self):
        return self.parent
        
    """
    Used for when the MCTS tree commits to a subtree.
    """
    def removeParent(self):
        self.parent = None
        
    """
    Return a list of child action nodes.
    """
    def getChildNodes(self):
        return self.actionChildren
        
    """
    Get the probability that this state occurs as a result
    of performing the parent action.
    """
    def getTransitionProbability(self):
        return self.transitionProbability
        
        
"""
The estimator provides us with two pieces of
information: the prior expert knowledge for
move selection (returning a prior probability
distribution over actions) which is an estimate
that MCTS aims to improve, and also provides
value estimates for given states.
These two methods are grouped into two objects
mainly because it is common for policy and
value networks to occupy these jobs, and they
usually share many neural network layers between
them.
"""
class EstimatorStrategy:
    """
    By simulation (or using a value function approximation),
    determine the value of the given game state.
    """
    def computeValueEstimate(self, rootState):
        pass
        
    """
    Return a list of probabilities which represent the probability
    that an expert will pick each action from the list 'actions'.
    Note that actions=state.getCurrentOptions().
    """
    def computePriorDistribution(self, state, actions):
        pass
        
        
"""
A policy strategy object is responsible for constructing
a distribution across actions, using statistics about
those actions, calculated by the MCTS tree.
"""
class PolicyStrategy:
    """
    A policy strategy determines the distribution over a list
    of actions. Given are:
    actionValues: the estimated value of each action (calculated as an
                  average over the values of their resulting states),
    actionVisitCounts: the number of times each action has been visited,
                       each contributing to a single simulation of that action.
    priorActionProbabilities: the initial distribution of actions, before simulation.
    actionTeam: the team which will be choosing the action
    ourTeam: the team performing the MCTS
    Return: a list of the same length which sums to 1, and gives us a distribution
            over which actions we should pick next. Note that if actionTeam != ourTeam
            you should be picking the WORSE action!
    """
    def getActionDistribution(self, actionValues, actionVisitCounts,\
        priorActionProbabilities, actionTeam, ourTeam):
        pass
        


class MCTS:
    """
    rootState : the state for MCTS to begin in.
    treePolicy : the policy which decides the distribution of actions from a leaf node
    finalPolicy : the policy which decides which action to take from the root node, after many simulations
    simStrategy : the strategy which decides the value of a state by simulation or otherwise.
    """
    def __init__(self, rootState, treePolicy, finalPolicy, simStrategy):
        self.root = StateNode(rootState)
        self.treePolicy = treePolicy
        self.finalPolicy = finalPolicy
        self.simStrategy = simStrategy
        self.team = rootState.getCurrentTeam()
        
    """
    Get the current distribution of actions, as
    informed by all of the simulations performed
    by MCTS.
    Returns [actions], [probabilities].
    """
    def getCurrentDistribution(self):
        actionNodes = self.root.getChildNodes()
        values = [node.getValueEstimate() for node in actionNodes]
        visitCounts = [node.getSampleCount() for node in actionNodes]
        priors = [node.getPrior() for node in actionNodes]
        dist = self.finalPolicy.getActionDistribution(values, visitCounts, priors, self.team, self.team)
        actions = [node.getActionCommand() for node in actionNodes]
        return actions,dist
        
    """
    Commit to a given action. This RE-ROOTS the MCTS tree
    so as not to waste any of the previous simulations which
    were used to inform the commit decision.
    i,j : the target position of the action you chose
    state : the state which resulted from applying that action
    This function will be checking that (i,j) was a valid action
    and that 'state' was a state which could actually result from
    that action.
    """
    def commit(self, i, j, state):
        #First, find action:
        actionNode = None
        for node in self.root.getChildNodes():
            if node.getActionCommand().getTargetPosition() == (i,j):
                actionNode = node
                break
        assert(actionNode != None)
        #Then, find state:
        stateNode = None
        for node in actionNode.getChildNodes():
            if node.getState() == state:
                stateNode = node
        assert(stateNode != None)
        #Then, remove state parent and update root pointer:
        stateNode.removeParent()
        self.root = stateNode
        
    """
    Simulate a further n times to improve the current action
    distribution estimate.
    """
    def simulate(self, n):
        #For each simulation...
        for i in range(n):
            curNode = self.root
            
            #Simulate until the end of our MCTS tree:
            while not curNode.isLeaf() and not curNode.isTerminal():
                #Obtain information about potential actions:
                actionNodes = curNode.getChildNodes()
                values = [node.getValueEstimate() for node in actionNodes]
                visitCounts = [node.getSampleCount() for node in actionNodes]
                priors = [node.getPrior() for node in actionNodes]
                
                #Compute distribution over actions:
                actionDist = self.treePolicy.getActionDistribution(values, visitCounts, priors, curNode.getState().getCurrentTeam(), self.team)
                #Select an action according to our tree policy:
                actionNode = selectRandomly(actionNodes, actionDist)
                #Select a state (via the random state transition dynamics):
                stateNode = selectRandomly(actionNode.getChildNodes(), actionNode.getChildNodeDistribution())
                #Update the node
                curNode = stateNode
                
            #Compute value estimate of this state
            valueEstimate = None
            if curNode.isTerminal():
                valueEstimate = curNode.getState().getGameValue()
            elif curNode.isLeaf():
                valueEstimate = self.simStrategy.computeValueEstimate(curNode.getState())
            assert(valueEstimate is not None)
            probability=1.0
            
            #Backpropagate results:
            while curNode != self.root:
                #Add the value statistic to the node:
                curNode.addStatistic(valueEstimate,probability)
                action = curNode.getParent()
                probability *= curNode.getTransitionProbability()
                action.updateEstimate()
                curNode = action.getParent()
                
            #And finally, add to root:
            self.root.addStatistic(valueEstimate,probability)
            #Done!
            
        
    """
    Get the current simulation count from the current tree root.
    NOTE: if you re-root the tree by calling commit(), this number
    might go down (because not all simulations would've taken the
    path that actually did occur).
    """
    def getCurrentSimulationCount(self):
        return self.root.getSampleCount()
        