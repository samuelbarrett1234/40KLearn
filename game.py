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
    (8,1,5,20)
]


model = Model(unitRoster, placements)
ctrl = TwoPlayerController(model,\
    BasicAIController(model),\
    HumanController(model)\
)
view = GameView(model, ctrl)

view.run()
