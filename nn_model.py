import tensorflow as tf
from tensorflow.keras import Model, Input
from tf.keras.layers import Conv2D, BatchNormalization, Dense, Flatten
from converter import NUM_FEATURES


BOARD_SIZE = 10
NUM_EPOCHS = 100


class NNModel:
    """
    This is the neural network model for predicting game values
    and for generating prior policies.
    """

    def __init__(self, filename=None):
        # Load from filename if given:
        if filename is not None:
            self.model = tf.keras.load_model(filename)
        else:
            self.board_input = Input(shape=(-1, BOARD_SIZE,
                                            BOARD_SIZE, NUM_FEATURES))

            # the current game's phase.
            self.phase_input = Input(shape=(-1, 4))

            # main neural network body
            x = Conv2D(256, (3, 3), activation='relu',
                       input_shape=self.board_input.shape[1:]
                       )(self.board_input)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Conv2D(256, (3, 3), activation='relu')(x)
            x = BatchNormalization()(x)
            x = Flatten()(x)
            # provide phase input here!
            x = Dense(256, activation='relu')(tf.concat([x,
                                                         self.phase_input]))
            x = BatchNormalization()(x)
            x = Dense(256, activation='relu')(x)
            x = BatchNormalization()(x)

            # construct value head of network
            self.value_head = Dense(128, activation='relu')(self.value_head)
            self.value_head = Dense(32, activation='relu')(self.value_head)
            self.value_head = Dense(1, activation='tanh')(self.value_head)
            # value head returns +1 for estimated win and -1 for estimated
            # loss, and all values inbetween

            # construct policy head of network
            self.policy_head = Dense(256,
                                     activation='relu')(self.policy_head)
            self.policy_head = Dense(256,
                                     activation='relu')(self.policy_head)
            self.policy_head = Dense(2 * BOARD_SIZE * BOARD_SIZE + 1,
                                     activation=None)(self.policy_head)
            # Explanation of the policy head output:
            # the first BOARD_SIZE * BOARD_SIZE elements represent the logit
            # probabilities of picking each position as the SOURCE position
            # for the next action. The next BOARD_SIZE * BOARD_SIZE elements
            # represent the logit probabilities of picking a particular element
            # as the TARGET position for the next action. The final element of
            # the array represents the logit of ending phase (a 'pass').

            # create model
            self.model = Model(inputs=[self.board_input, self.phase_input],
                               outputs=[self.value_head, self.policy_head])

            # set up optimisers/losses
            optimiser = 'adam'
            value_head_loss = 'mse'
            policy_head_loss = 'crossentropy'

            # compile model
            self.model.compile(optimizer=optimiser,
                               loss=[value_head_loss, policy_head_loss])

    def train(self, input_array, values, policies):
        # input array: should be of the form <list of board representations as
        # tensors, list of phases as vectors>
        # values: should be a list of scalars
        # policies: should be a list of 2D arrays of the same
        # size as 2 * BOARD_SIZE * BOARD_SIZE + 1
        self.model.fit(input_array, [values, policies], epochs=NUM_EPOCHS)

    def run(self, input_array):
        # input array: should be of the form <list of board representations as
        # tensors, list of phases as vectors>
        # values: will be a list of scalars
        # policies: will be a list of 2D arrays of the same
        # size as 2 * BOARD_SIZE * BOARD_SIZE + 1
        values, policies = self.model.predict(input_array)
        return values, policies

    def save(self, filename):
        self.model.save(filename)
