from view import GameView
from human_controller import HumanController
from model import Model
from game_util import *


unitRoster = loadUnitsCSV("unit_stats.csv")
placements = [
    (0,0,5,5),
    (0,0,20,5),
    (2,0,10,5),
    (2,0,15,5),
    (3,0,12,7),
    (5,0,11,7),
    (3,0,5,10),
    (3,0,10,10),
    (3,0,15,10),
    (3,0,20,10),
    (6,1,5,20),
    (6,1,20,20),
    (7,1,10,20),
    (7,1,15,20),
    (8,1,12,18)
]


model = Model(unitRoster, placements)
ctrl = HumanController(model)
view = GameView(model, ctrl)

view.run()
