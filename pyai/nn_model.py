import tensorflow as tf
from tensorflow.keras import Model, Input
from tensorflow.keras.layers import (Conv2D, BatchNormalization, Dense,
                                     Flatten, concatenate)
from pyai.converter import NUM_FEATURES


tf.logging.set_verbosity(tf.logging.WARN)  # don't print info logs


class NNModel:
    """
    This is the neural network model for predicting game values
    and for generating prior policies.
    """

    def __init__(self, board_size, num_epochs, filename=None):
        self.board_size = board_size
        self.num_epochs = num_epochs

        self.board_input = Input(shape=(self.board_size,
                                        self.board_size, 2 * NUM_FEATURES))

        # the current game's phase.
        self.phase_input = Input(shape=(4,))

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
        x = Conv2D(256, (3, 3), activation='relu')(x)
        x = BatchNormalization()(x)
        x = Flatten()(x)  # flatten CNN output
        x = concatenate([x, self.phase_input])  # add phase input
        x = Dense(256, activation='relu')(x)
        x = BatchNormalization()(x)
        x = Dense(256, activation='relu')(x)

        # construct value head of network
        self.value_head = Dense(256, activation='relu')(x)
        self.value_head = Dense(256, activation='relu')(self.value_head)
        self.value_head = Dense(32, activation='relu')(self.value_head)
        self.value_head = Dense(1, activation='tanh')(self.value_head)
        # value head returns +1 for estimated win and -1 for estimated
        # loss, and all values inbetween

        # construct policy head of network
        self.policy_head = Dense(256,
                                 activation='relu')(x)
        self.policy_head = Dense(256,
                                 activation='relu')(self.policy_head)
        self.policy_head = Dense(256,
                                 activation='relu')(self.policy_head)
        self.policy_head = Dense(2 * self.board_size * self.board_size + 1,
                                 activation='softmax')(self.policy_head)

        # Explanation of the policy head output:
        # the first board_size * board_size elements represent the
        # probabilities of picking each position as the SOURCE position
        # for the next action. The next board_size * board_size elements
        # represent the probabilities of picking a particular element
        # as the TARGET position for the next action. The final element of
        # the array represents the probability of ending phase (a 'pass').

        # create model
        self.model = Model(inputs=[self.board_input, self.phase_input],
                           outputs=[self.value_head, self.policy_head])

        # set up optimisers/losses
        optimiser = 'adam'
        value_head_loss = 'mse'
        policy_head_loss = 'categorical_crossentropy'

        # compile model
        self.model.compile(optimizer=optimiser,
                           loss=[value_head_loss, policy_head_loss])

        # Load weights from filename if given:
        if filename is not None:
            self.model.load_weights(filename)

    def train(self, input_game_states, input_phases, values, policies):
        # input game states and phases: should be in array form
        # values: should be a list of scalars
        # policies: should be a list of 2D arrays of the same
        # size as 2 * self.board_size * self.board_size + 1

        assert(len(input_game_states) == len(input_phases))
        assert(len(input_game_states) == len(values))
        assert(len(input_game_states) == len(policies))
        assert(all([len(p) == 2 * self.board_size * self.board_size + 1
                    for p in policies]))
        assert(all([len(ph) == 4 for ph in input_phases]))

        self.model.fit([input_game_states, input_phases], [values, policies],
                       epochs=self.num_epochs)

    def predict(self, input_game_states, input_phases):
        # input game states and phases: should be in array form
        # values: will be a list of scalars
        # policies: will be a list of 2D arrays of the same
        # size as 2 * self.board_size * self.board_size + 1

        assert(len(input_game_states) == len(input_phases))
        assert(all([len(ph) == 4 for ph in input_phases]))

        values, policies = self.model.predict([input_game_states,
                                               input_phases])
        return values, policies

    def save(self, filename):
        self.model.save_weights(filename)
