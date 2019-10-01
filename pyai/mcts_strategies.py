
class VisitCountStochasticPolicyStrategy:
    def __init__(self, tau):
        assert(tau > 0.0)
        self.recip_tau = 1.0 / tau

    def get_action_distribution(self, cur_node):
        visit_counts = cur_node.get_action_visit_counts()
        unnormalised = [n ** self.recip_tau for n in visit_counts]
        s = sum(unnormalised)
        if s == 0.0:
            return [1.0 / len(visit_counts) for i in visit_counts]
        else:
            return [p/s for p in unnormalised]
