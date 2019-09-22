from pyapp.view import GameView
from pyapp.human_controller import HumanController
from pyapp.two_player_controller import TwoPlayerController
from pyai.basic_ai_controller import BasicAIController
from pyapp.model import Model
from pyapp.game_util import load_units_csv


# Player versus player or AI
PVP = 0
PVAI = 1
# Game mode:
MODE = PVAI


unit_roster = load_units_csv("unit_stats.csv")
placements = [
    (4, 0, 15, 22),
    (0, 0, 10, 20),
    (0, 0, 20, 20),
    (2, 0, 12, 20),
    (2, 0, 18, 20),
    (1, 0, 15, 20),
    (7, 1, 10, 10),
    (7, 1, 20, 10),
    (8, 1, 12, 10),
    (8, 1, 18, 10),
    (9, 1, 15, 10),
    (13, 1, 17, 10)
]


model = Model(unit_roster, placements)

if __name__ == "__main__":
    team0 = HumanController(model)
    team1 = None
    if MODE == PVP:
        team1 = HumanController(model)
    elif MODE == PVAI:
        team1 = BasicAIController(model)
    ctrl = TwoPlayerController(model, team0, team1)
    view = GameView(model, ctrl)
    view.run()
