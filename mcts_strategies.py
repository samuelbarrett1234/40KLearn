import random
import math
from game_util import selectRandomly


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
        priorActionProbabilities, actionTeam, ourTeam, curPhase):
        pass



class UniformRandomEstimatorStrategy(EstimatorStrategy):
    def __init__(self,team):
        self.team=team
        
    def computeValueEstimate(self, rootState):
        #Compute value of game by uniform random simulation
        state = rootState.createCopy()
        while not state.finished():
            actions = state.getCurrentOptions()
            action = actions[random.randint(0, len(actions)-1)]
            results,probs = state.chooseOption(action)
            state = selectRandomly(results,probs)
        gameVal = state.getGameValue(self.team)
        return gameVal
        
    def computePriorDistribution(self, state, actions):
        #Just return uniform distribution:
        N = len(actions)
        return [1 / N for a in actions]
        
        
class UCB1PolicyStrategy(PolicyStrategy):
    """
    c : exploratory parameter
    """
    def __init__(self, c):
        assert(c > 0.0)
        self.c = c
        
    def getActionDistribution(self, actionValues, actionVisitCounts,\
        priors, actionTeam, ourTeam, curPhase):
        #N is the number of simulations ran from the parent state
        N = sum(actionVisitCounts)
        assert(N>0)
        
        #The team value is multiplied with all of the
        # action values before taking the max, so a value
        # of -1 effectively makes us choose the min.
        teamValue = 1.0
        if ourTeam != actionTeam:
            teamValue = -1.0
        
        #Compute UCB1 values for each action
        ucbValues = [teamValue*q+self.c*p*(math.log(N)/(1+n))**0.5 \
            for q,p,n in zip(actionValues,priors,actionVisitCounts)]
        
        #Find max and pick that action (deterministically):
        iMax = None
        maxValue = -2.0
        for i in range(len(ucbValues)):
            if ucbValues[i] > maxValue:
                iMax = i
                maxValue = ucbValues[i]
        
        assert(iMax is not None)
        #Return a distribution with 1 in the position of the max
        # and 0 everywhere else
        return [float(i==iMax) for i in range(len(ucbValues))]
        
        
        
class VisitCountStochasticPolicyStrategy:
    def __init__(self, tau):
        assert(tau > 0.0)
        self.recip_tau = 1.0 / tau
        
    def getActionDistribution(self, actionValues, actionVisitCounts,\
        priors, actionTeam, ourTeam, curPhase):
        unnormalised = [n ** self.recip_tau for n in actionVisitCounts]
        s = sum(unnormalised)
        assert(s>0.0)
        return [p/s for p in unnormalised]
        
        
        