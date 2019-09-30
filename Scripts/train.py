import os
from typing import Union
from glob import iglob
from argparse import ArgumentParser
from pyai.nn_model import NNModel
from pyai.experience_dataset import ExperienceDataset
from pyai.converter import NUM_FEATURES
from pyapp.model import BOARD_SIZE


if __name__ == "__main__":
    ap = ArgumentParser()
    ap.add_argument("--model_filename",
                    help=("The .h5 file containing the model weights. "
                          "If None, then will start from scratch."),
                    type=Union[str, None],
                    default=None)
    ap.add_argument("--data",
                    help="The wildcard pattern for all data folders.",
                    type=str, required=True)
    ap.add_argument("--num_batches",
                    help=("The number of batches to run. Each batch "
                          "is formed of a new sample. The neural"
                          "network then trains for several epochs on "
                          "that batch."),
                    type=int,
                    default=10)
    ap.add_argument("--batch_size",
                    help="The number of samples in each batch.",
                    type=int,
                    default=1000)
    ap.add_argument("--num_epochs",
                    help=("The number of training iterations on a single"
                          "batch."),
                    type=int,
                    default=50)

    ap.parse_args()

    # Check arguments
    if not((ap.model_filename is None or
            os.path.exists(ap.model_filename)) and
           len(iglob(ap.data)) > 0 and
           ap.num_batches > 0 and
           ap.batch_size > 0 and
           ap.num_epochs > 0):
        raise ValueError("Invalid command line arguments.")

    # Create the neural network model:
    model = NNModel(num_epochs=ap.num_epochs, board_size=BOARD_SIZE,
                    filename=(ap.model_filename if
                              os.path.exists(ap.model_filename)
                              else None))

    # Create the dataset:
    dataset = ExperienceDataset(filename=ap.data,
                                board_size=BOARD_SIZE,
                                num_board_features=NUM_FEATURES)

    for i in range(ap.num_batches):
        print("*** Starting batch", i + 1, "... collecting sample...")
        # Obtain batch sample:
        (game_states, phases, values,
         policies) = dataset.sample(ap.batch_size)

        print("*** Dataset constructed. Training model...")

        # Now ready to perform a training epoch on the model:
        model.train(game_states, phases, values, policies)

        print("*** Finished training batch, saving model...")

        model.save(ap.model_filename)  # save after each self-play epoch

    print("*** DONE!")
