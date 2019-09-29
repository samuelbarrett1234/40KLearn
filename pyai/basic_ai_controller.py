from pyai.mcts import MCTS
from pyai.mcts_strategies import (UniformRandomEstimatorStrategy,
                                  VisitCountStochasticPolicyStrategy)
from pyapp.game_util import select_randomly
import py40kl


class BasicAIController:
    """
    This AI controller uses MCTS without neural networks; in particular,
    it just uses UCB1 with a uniform simulation strategy until the game
    ends.
    """

    def __init__(self, model):
        self.model = model
        self.tau = 0.2
        self.exploratoryParam = 2.0 ** 0.5
        self.N = 25
        self.tree = None
        self.on_turn_changed()  # Sets up the MCTS tree

    def on_update(self):
        # Simulate:
        self.tree.simulate(self.N - self.tree.get_num_samples())

        # Get results:
        actions, dist = self.tree.get_distribution()

        # Select and apply:
        action = select_randomly(actions, dist)
        if action.get_type() != py40kl.CommandType.UNIT_ORDER:
            # Log what happened
            if len(actions) > 1:
                print("AI decided to end turn/phase")
            else:
                print("AI ended turn (no other possible actions.)")
        else:
            # Determine target position:
            target_pos = action.get_target_position()

            # Determine source position:
            source_pos = action.get_source_position()

            # Cache board state
            board = self.model.get_state().get_board_state()

            # Log what happened:
            unit = board.get_unit_on_square(source_pos)
            verb, subject = "", ""
            if self.model.get_phase() == py40kl.Phase.MOVEMENT:
                verb = "move to"
                subject = str((target_pos.x, target_pos.y))
            elif self.model.get_phase() == py40kl.Phase.SHOOTING:
                verb = "shoot"
                target = board.get_unit_on_square(target_pos)
                subject = target.name
            elif self.model.get_phase() == py40kl.Phase.CHARGE:
                verb = "charge location"
                subject = str((target_pos.x, target_pos.y))
            else:
                verb = "fight"
                target = board.get_unit_on_square(target_pos)
                subject = target.name

            print("AI decided for", unit.name, "to", verb, subject)

        # Actually apply the changes
        self.model.choose_action(action)
        self.tree.commit(self.model.get_state())

    def on_click_position(self, pos, bLeft):
        pass  # AI doesn't care about clicks

    def on_return(self):
        pass  # AI doesn't care about clicks

    def on_turn_changed(self):
        # Reconstruct MCTS tree every turn

        acting_team = self.model.get_acting_team()

        # MCTS components:
        rootState = self.model.get_state()
        treePolicy = py40kl.UCB1PolicyStrategy(self.exploratoryParam,
                                               acting_team)
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy(acting_team)

        # Create MCTS tree
        self.tree = MCTS(rootState, treePolicy, finalPolicy, simStrategy)
