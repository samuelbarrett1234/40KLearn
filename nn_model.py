import tensorflow as tf
from tensorflow.keras import layers


BOARD_SIZE = 25
NUM_UNIT_FEATURES = 11 # todo - what is the correct value for this?
NUM_EPOCHS = 100


"""
This is the neural network model for predicting game values
and for generating prior policies.
"""
class NNModel:
    def __init__(self):
        self.board_input = tf.keras.Input(shape=(-1, BOARD_SIZE, BOARD_SIZE, NUM_UNIT_FEATURES))
        self.phase_input = tf.keras.Input(shape=(-1, 4))  # the current game's phase.
        
        # main neural network body
        x = layers.Conv2D(256, (3, 3), activation='relu', input_shape=self.board_input.shape[1:])(self.board_input)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Conv2D(256, (3, 3), activation='relu')(x)
        x = layers.BatchNormalization()(x)
        x = layers.Flatten()(x)
        x = layers.Dense(256, activation='relu')(tf.concat([x, self.phase_input]))  # provide phase input here!
        
        # construct value head of network
        self.value_head = layers.BatchNormalization()(self.value_head)
        self.value_head = layers.Dense(128, activation='relu')(self.value_head)
        self.value_head = layers.Dense(1, activation='tanh')(self.value_head)
        # value head returns +1 for estimated win and -1 for estimated loss, and all values inbetween
        
        # construct policy head of network
        self.policy_head = layers.BatchNormalization()(self.policy_head)
        self.policy_head = layers.Dense(256, activation='relu')(self.policy_head)
        self.policy_head = layers.Dense(BOARD_SIZE * BOARD_SIZE + 1, activation='softmax')(self.policy_head)
        # policy head returns target action position probabilities, and an extra one for pass
        
        # create model
        self.model = tf.keras.Model(inputs=self.input, outputs=[self.value_head, self.policy_head])
        
        # set up optimisers/losses
        optimiser = 'adam'
        value_head_loss = 'mse'
        policy_head_loss = 'crossentropy'
        
        # compile model
        self.model.compile(optimizer=optimiser,
                           loss=[value_head_loss, policy_head_loss])

    def train(self, input_array, values, policies):
        """
        Train the neural network model on the given input array
        (which is a list of game states which have been converted
        to tensors) and a list of values (corresponding to each
        game state) and a list of policies (again, corresponding
        to each game state).
        """
        # fit model
        self.model.fit(input_array, [values, policies], epochs=NUM_EPOCHS)
        # done!

    def run(self, input_array):
        # predict the value and policy 
        values, policies = self.model.predict(input_array)
        return values, policies
