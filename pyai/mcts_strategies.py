import random
import math
from game_util import select_randomly
import py40kl



class UniformRandomEstimatorStrategy:
    def __init__(self,team):
        self.team = team
        self.estimator = py40kl.UniformRandomEstimator()
        #The number of simulations to perform when estimating values:
        self.num_sims = 3
        
    def compute_value_estimate(self, state):
        return self.estimator.compute_value_estimate(state, self.team, self.num_sims)
        
    def compute_prior_distribution(self, state, actions):
        #Just return uniform distribution:
        N = len(actions)
        return [1 / N for a in actions]



class VisitCountStochasticPolicyStrategy:
    def __init__(self, tau):
        assert(tau > 0.0)
        self.recip_tau = 1.0 / tau
        
    def get_action_distribution(self, cur_node):
        visit_counts = cur_node.get_action_visit_counts()        
        unnormalised = [n ** self.recip_tau for n in visit_counts]
        s = sum(unnormalised)
        assert(s>0.0)
        return [p/s for p in unnormalised]
        
        
        