from pyapp.view import GameView
from pyapp.human_controller import HumanController
from pyapp.two_player_controller import TwoPlayerController
from pyai.nn_ai_controller import NeuralNetworkAIController
from pyapp.model import Model
from pyapp.game_util import load_units_csv
from train import MODEL_FILENAME


# Player versus player or AI
PVP = 0
PVAI = 1
# Game mode:
MODE = PVAI


unit_roster = load_units_csv("unit_stats.csv")
placements = [
    (0, 0, 15, 5),
    (1, 0, 15, 10),
    (2, 0, 15, 15),
    (7, 1, 5, 5),
    (8, 1, 5, 10),
    (9, 1, 5, 15),
]


model = Model(unit_roster, placements)

if __name__ == "__main__":
    team0 = HumanController(model)
    team1 = None
    if MODE == PVP:
        team1 = HumanController(model)
    elif MODE == PVAI:
        team1 = NeuralNetworkAIController(model,
                                          MODEL_FILENAME)
    ctrl = TwoPlayerController(model, team0, team1)
    view = GameView(model, ctrl)
    view.run()
