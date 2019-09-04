import random
import math
from game_util import select_randomly
import py40kl



class UniformRandomEstimatorStrategy:
    def __init__(self,team):
        self.team=team
        
    def compute_value_estimate(self, state):
        #Compute value of game by uniform random simulation
        while not state.is_finished():
            actions = state.get_commands()
            action = actions[random.randint(0, len(actions)-1)]
            results, probs = py40kl.GameStateArray(), py40kl.FloatArray()
            action.apply(state, results, probs)
            state = select_randomly(results, probs)
        gameVal = state.get_game_value(self.team)
        return gameVal
        
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
        
        
        