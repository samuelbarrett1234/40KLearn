import random
import math
from game_util import selectRandomly



class UniformRandomEstimatorStrategy:
    def __init__(self,team):
        self.team=team
        
    def compute_value_estimate(self, rootState):
        #Compute value of game by uniform random simulation
        state = rootState.createCopy()
        while not state.finished():
            actions = state.getCurrentOptions()
            action = actions[random.randint(0, len(actions)-1)]
            results,probs = state.chooseOption(action)
            state = selectRandomly(results,probs)
        gameVal = state.getGameValue(self.team)
        return gameVal
        
    def compute_prior_distribution(self, state, actions):
        #Just return uniform distribution:
        N = len(actions)
        return [1 / N for a in actions]
              
              
        
class VisitCountStochasticPolicyStrategy:
    def __init__(self, tau):
        assert(tau > 0.0)
        self.recip_tau = 1.0 / tau
        
    def get_action_distribution(self, action_values, action_visit_counts, priors):
        unnormalised = [n ** self.recip_tau for n in action_visit_counts]
        s = sum(unnormalised)
        assert(s>0.0)
        return [p/s for p in unnormalised]
        
        
        