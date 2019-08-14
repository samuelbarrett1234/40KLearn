from view import GameView
from human_controller import HumanController
from two_player_controller import TwoPlayerController
from basic_ai_controller import BasicAIController
from model import Model
from game_util import *


unitRoster = loadUnitsCSV("unit_stats.csv")
placements = [
    (0,0,5,5),
    (0,0,20,5),
    (2,0,10,5),
    (2,0,15,5),
    (1,0,11,6),
    (6,0,12,8),
    (7,1,5,20),
    (7,1,20,20),
    (8,1,10,20),
    (8,1,15,20),
    (9,1,12,18)
]


model = Model(unitRoster, placements)
ctrl = TwoPlayerController(model\
    HumanController(model),\
    BasicAIController(model)\
)
view = GameView(model, ctrl)

view.run()
