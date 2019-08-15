from view import GameView
from human_controller import HumanController
from two_player_controller import TwoPlayerController
from basic_ai_controller import BasicAIController
from model import Model
from game_util import *


PVP = 0
PVAI = 1
AI_ONLY = 2

mode = PVAI


unitRoster = loadUnitsCSV("unit_stats.csv")
placements = [
    (4,0,15,22),
    (0,0,10,20),
    (0,0,20,20),
    (2,0,12,20),
    (2,0,18,20),
    (1,0,15,20),
    (7,1,10,10),
    (7,1,20,10),
    (8,1,12,10),
    (8,1,18,10),
    (9,1,15,10),
    (13,1,17,10),
]


model = Model(unitRoster, placements)

if mode == AI_ONLY:
    ctrl = BasicAIController(model)
    ctrl.onUpdate()

    print("Done!")
else:
    team0 = HumanController(model)
    team1 = None
    if mode == PVP:
        team1 = HumanController(model)
    elif mode == PVAI:
        team1 = BasicAIController(model)
    ctrl = TwoPlayerController(model, team0, team1)
    view = GameView(model, ctrl)
    view.run()


