from mcts_strategies import EstimatorStrategy, PolicyStrategy,\
                            UniformRandomEstimatorStrategy, UCB1PolicyStrategy,\
                            VisitCountStochasticPolicyStrategy
from game_util import selectRandomly

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
        states, self.probs = parentStateNode.getState().chooseOption(actionCmd)
        self.stateNodeChildren = [StateNode(state, self, prob) for state,prob in zip(states,self.probs)]
        #We will cache our current value estimate of this action, derived
        # from the child states' estimates.
        self.currentEstimate = 0
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
    def __init__(self, state, actionParent=None, transitionProbability=1.0):
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
        return self.meanValue
        
    """
    Get the action node which lead to this state, or None
    if this is the root node.
    """
    def getParent(self):
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
        
    def getState(self):
        return self.state
        
        

        


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
        dist = self.finalPolicy.getActionDistribution(values, visitCounts, priors,\
            self.team, self.team, self.root.getState().getPhase())
        actions = [node.getActionCommand() for node in actionNodes]
        return actions,dist
        
    """
    Commit to a given action. This RE-ROOTS the MCTS tree
    so as not to waste any of the previous simulations which
    were used to inform the commit decision.
    pos=(i,j) : the target position of the action you chose
    state : the state which resulted from applying that action
    This function will be checking that (i,j) was a valid action
    and that 'state' was a state which could actually result from
    that action.
    """
    def commit(self, pos, state):
        #First, find action:
        actionNode = None
        for node in self.root.getChildNodes():
            if node.getActionCommand().getTargetPosition() == pos:
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
                actionDist = self.treePolicy.getActionDistribution(values, visitCounts, priors,\
                    curNode.getState().getCurrentTeam(), self.team, curNode.getState().getPhase())
                #Select an action according to our tree policy:
                actionNode = selectRandomly(actionNodes, actionDist)
                #Select a state (via the random state transition dynamics)
                # and then update the current node
                curNode = selectRandomly(actionNode.getChildNodes(), actionNode.getChildNodeDistribution())
                
            #Compute value estimate of this state
            valueEstimate = None
            if curNode.isTerminal():
                valueEstimate = curNode.getState().getGameValue()
            elif curNode.isLeaf():
                #Expand the leaf node BEFORE doing a simulation
                #This means we need to get actions and prior probabilities
                actions = curNode.getState().getCurrentOptions()
                priors = self.simStrategy.computePriorDistribution(curNode.getState(), actions)
                curNode.expand(actions,priors)
                actionNodes = curNode.getChildNodes()
                #Apply an action sampled from the prior:
                actionNode = selectRandomly(actionNodes,priors)
                #Select a state (via the random state transition dynamics)
                # and then update the current node
                curNode = selectRandomly(actionNode.getChildNodes(), actionNode.getChildNodeDistribution())
                #Simulate to compute its value:
                valueEstimate = self.simStrategy.computeValueEstimate(curNode.getState())
                #We will then backpropagate this valule below
                
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
        