import os
from argparse import ArgumentParser
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
                    help=("The .h5 file containing the model weights. If file "
                          "doesn't exist, will start training from scratch."),
                    type=str, required=True)
    ap.add_argument("--initial_state",
                    help=("The CSV file of the initial state to play in."),
                    type=str, required=True)
    ap.add_argument("--unit_data",
                    help="The table of unit statistics.",
                    type=str, required=True)
    ap.add_argument("--turn_limit",
                    help=("The maximum number of turns per game. "
                          "Negative numbers imply no turn limit."),
                    type=int,
                    default=6)
    ap.add_argument("--team0",
                    help="Who is playing as team 0 (P or AI).",
                    type=str,
                    default="P")
    ap.add_argument("--team1",
                    help="Who is playing as team 1 (P or AI).",
                    type=str,
                    default="AI")

    args = ap.parse_args()

    # Check arguments
    if not(os.path.exists(args.model_filename) and
            os.path.exists(args.initial_state) and
            os.path.exists(args.unit_data) and
            args.turn_limit != 0 and
            args.team0 in ["P", "AI"] and
            args.team1 in ["P", "AI"]):
        raise ValueError("Invalid command line arguments.")

    # Load the unit statistics dataset:
    units_dataset = load_units_csv(args.unit_data)
    # Extract a separate list of names:
    unit_names = [x["name"] for x in units_dataset]
    unit_placements = load_unit_placements_csv(args.initial_state)
    # Need to convert name column to index column:
    unit_placements = [(unit_names.index(n), t, x, y)
                       for n, t, x, y in unit_placements]

    # Check all names in the map file were valid:
    assert(all([i >= 0 and i < len(units_dataset)
                for i, _, __, ___ in unit_placements]))

    model = Model(units_dataset, unit_placements)

    team0 = create_controller(args.team0, model, args.model_filename)
    team1 = create_controller(args.team1, model, args.model_filename)

    ctrl = TwoPlayerController(model, team0, team1)
    view = GameView(model, ctrl)
    view.run()
