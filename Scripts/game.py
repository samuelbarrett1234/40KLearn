import os
from random import choice
from argparse import ArgumentParser
from typing import Union
from glob import iglob
from pyapp.view import GameView
from pyapp.human_controller import HumanController
from pyapp.two_player_controller import TwoPlayerController
from pyai.nn_ai_controller import NeuralNetworkAIController
from pyapp.model import Model
from pyapp.game_util import load_units_csv, load_unit_placements_csv


def create_controller(ctrl_type, model,
                      model_filename):
    if ctrl_type == "P":
        return HumanController(model)
    else:
        assert(ctrl_type == "AI")
        return NeuralNetworkAIController(model, model_filename)


if __name__ == "__main__":
    ap = ArgumentParser()
    ap.add_argument("--model_filename",
                    help=("The .h5 file containing the model weights. "
                          "If None, then will start from scratch."),
                    type=Union[str, None],
                    default=None)
    ap.add_argument("--turn_limit",
                    help=("The maximum number of turns per game. "
                          "Negative numbers imply no turn limit."),
                    type=int,
                    default=6)
    ap.add_argument("--initial_states",
                    help=("The wildcard pattern for all CSV files of"
                          " 'unit placement tables'; at each iteration"
                          " one of these will be selected at random."),
                    type=str, required=True)
    ap.add_argument("--unit_data",
                    help="The table of unit statistics.",
                    type=str, required=True)
    ap.add_argument("--team0",
                    help="Who is playing as team 0 (P or AI).",
                    type=str,
                    default="P")
    ap.add_argument("--team1",
                    help="Who is playing as team 1 (P or AI).",
                    type=str,
                    default="AI")

    ap.parse_args()

    # Check arguments
    if not((ap.model_filename is None or
            os.path.exists(ap.model_filename)) and
            ap.turn_limit != 0 and
            len(iglob(ap.initial_states)) > 0 and
            os.path.exists(ap.unit_data) and
            ap.team0 in ["P", "AI"] and
            ap.team1 in ["P", "AI"]):
        raise ValueError("Invalid command line arguments.")

    # Load the unit statistics dataset:
    units_dataset = load_units_csv(ap.unit_data)
    # Extract a separate list of names:
    unit_names = [x[0] for x in units_dataset]
    unit_placements_fname = choice(iglob(ap.initial_states))
    unit_placements = load_unit_placements_csv(unit_placements_fname)
    # Need to convert name column to index column:
    unit_placements = [(unit_names.index(n), t, x, y)
                       for n, t, x, y in unit_placements]

    # Check all names in the map file were valid:
    assert(all([i >= 0 and i < len(units_dataset)
                for i, _, __, ___ in unit_placements]))

    model = Model(units_dataset, unit_placements)

    team0 = create_controller(ap.team0, model, ap.model_filename)
    team1 = create_controller(ap.team0, model, ap.model_filename)

    ctrl = TwoPlayerController(model, team0, team1)
    view = GameView(model, ctrl)
    view.run()
