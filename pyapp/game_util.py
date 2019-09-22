import random
import csv
import py40kl


# The names and order of the unit stats
columns = [
    # NOTE: no randomness in the unit stats allowed!
    "name",
    # The number of models left in the squad
    # (all members of squad must be identical)
    "count",
    # The unit's statistics
    "movement",
    "ws",
    "bs",
    # NOTE: we do not have strength here because it is stored in ml_s
    "t",
    "w",  # This is wounds per model, however THIS IS NEVER UPDATED.
    "total_w",  # Note that this is total squad wounds, not per model
    "a",
    "ld",
    "sv",
    "inv",
    # Ranged weapon stats
    "rg_range",
    "rg_s",
    "rg_ap",
    "rg_dmg",
    "rg_shots",
    "rg_is_rapid",
    "rg_is_heavy",  # Make true if you want an accuracy penalty for moving
    # Melee weapon stats
    "ml_s",
    "ml_ap",
    "ml_dmg",
    # Special rules (1 represents having the rule, 0 means not)
    # (none as of yet)
]


def load_units_csv(filename):
    """
    A helper function for loading a unit database CSV file in
    and returning a list of stat dictionaries.
    """
    units = []
    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        isHeader = True
        colHeaders = []
        for row in csv_reader:
            if isHeader:
                colHeaders = row
                isHeader = False
            else:
                u = {}
                for i, entry in enumerate(row):
                    col = colHeaders[i]
                    if col != "name":
                        entry = int(entry)  # Convert datatype if not unit name
                    u[col] = entry
                units.append(u)
    return units


def select_randomly(results, probs):
    """
    A helper function for selecting a result randomly
    based on an array of corresponding probabilities.
    """
    assert(len(results) == len(probs))
    r = random.random()
    total = 0.0
    for i, p in enumerate(probs):
        total += p
        if total > r:
            return results[i]
    assert(False and "Should never reach here")


def new_game_state(unitRoster, placements, board_size):
    """
    Create a new game with the given unit roster (array of
    units) and placements (a list of tuples (unit_idx, team, x, y)).
    """
    b = py40kl.BoardState(board_size, board_size)
    for unitIdx, team, x, y in placements:
        unit = unitRoster[unitIdx]
        pos = py40kl.Position(x, y)
        newunit = py40kl.Unit()

        newunit.name = unit["name"]
        newunit.count = unit["count"]
        newunit.movement = unit["movement"]
        newunit.ws = unit["ws"]
        newunit.bs = unit["bs"]
        newunit.t = unit["t"]
        newunit.w = unit["w"]
        newunit.total_w = unit["w"]*unit["count"]
        newunit.a = unit["a"]
        newunit.ld = unit["ld"]
        newunit.sv = unit["sv"]
        newunit.inv = unit["inv"]
        newunit.rg_range = unit["rg_range"]
        newunit.rg_s = unit["rg_s"]
        newunit.rg_ap = unit["rg_ap"]
        newunit.rg_dmg = unit["rg_dmg"]
        newunit.rg_shots = unit["rg_shots"]
        newunit.ml_s = unit["ml_s"]
        newunit.ml_ap = unit["ml_ap"]
        newunit.ml_dmg = unit["ml_dmg"]
        newunit.rg_is_rapid = unit["rg_is_rapid"]
        newunit.rg_is_heavy = unit["rg_is_heavy"]

        b.set_unit_on_square(pos, newunit, team)

    return py40kl.GameState(0, 0, py40kl.Phase.MOVEMENT, b)
