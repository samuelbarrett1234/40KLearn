from view import GameView
from human_controller import HumanController
from two_player_controller import TwoPlayerController
from basic_ai_controller import BasicAIController
from model import Model
from game_util import *


unitRoster = loadUnitsCSV("unit_stats.csv")
placements = [
    (9,0,15,17),
    (4,1,15,22)
]


model = Model(unitRoster, placements)
ctrl = BasicAIController(model)
ctrl.onUpdate()

print("Done!")

"""
ctrl = TwoPlayerController(model,\
    BasicAIController(model),\
    HumanController(model)\
)
view = GameView(model, ctrl)

view.run()
"""