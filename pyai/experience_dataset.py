import numpy as np
import pandas as pd
import dask.dataframe as dd
import os


class ExperienceDataset:
    """
    This class is used for recording "experiences" for the AI to learn
    from, so they can be used multiple times. This stores experiences
    in several files. It does this by creating a pandas dataframe for
    each commit(), saving in the Parquet file format, and then uses the
    Dask library to read all of these in at once and create a sample from
    them. We thus never store the experience dataset in memory in large
    proportions.
    """

    def __init__(self, filename, board_size, num_board_features):
        assert('*' in filename)  # need wildcard for the index

        self.filename = filename

        # Need these parameters so we know how big the game state
        # array should be.
        self.board_size = board_size
        self.num_board_features = num_board_features

        self.buffer_gs = []  # list of game states per buffer element
        self.buffer_ph = []  # list of phases as vectors per buffer element
        self.buffer_pol = []  # list of policies per buffer element
        self.buffer_gs_teams = []  # list: acting team for each game state

        self.buffer_size = 0  # number of games being fed into buffer

    def sample(self, n):
        # returns game_states (as arrays), phase vectors, values, policies

        # Load all data (lazily)
        dataset = dd.read_parquet(self.filename)

        # unfortunately Dask does not support drawing a specific number of
        # samples from dataset. Instead, sample 10% and trim to the given
        # size (bit of a hacky workaround, though.)
        sample = np.empty((0, 4))
        while len(sample) < n:
            s = dataset.sample(frac=0.1, replace=True)
            if s.shape[1] > 4:
                s = np.delete(np.array(s.values), 4, 1)  # trim last column
            sample = np.concatenate((sample, s), axis=0)

        # Clip to correct size if we went over:
        sz = min(n, sample.shape[0])
        sample = sample[:sz]

        # Compute shape of game state arrays:
        gs_shape = (self.board_size, self.board_size,
                    self.num_board_features * 2)

        # Note that each entry in the sample array is a string which
        # needs converting into a numpy array. Also, fromstring interprets
        # as a 1D array, so we need to reshape the game state elements to
        # make them the correct shape.
        states = np.array([np.fromstring(x, sep=',').reshape(gs_shape)
                           for x in sample.T[0]])
        values = np.array(list(sample.T[1]))  # list of floats so this is fine
        policies = np.array([np.fromstring(x, sep=',') for x in sample.T[2]])
        phases = np.array([np.fromstring(x, sep=',') for x in sample.T[3]])

        return states, phases, values, policies

    def set_buffer(self, n):
        self.buffer_gs = [[] for i in range(n)]
        self.buffer_pol = [[] for i in range(n)]
        self.buffer_ph = [[] for i in range(n)]
        self.buffer_gs_teams = [[] for i in range(n)]
        self.buffer_size = n

    def commit(self, values):
        """
        Write all of the values in the buffer to our dataset. Furthermore,
        we are provided with the game's end state (the value) for each of
        the games we've been recording. This game value is with respect to
        team 0, so we will need to multiply it by -1 for about half of our
        recordings.
        So if we are recording 10 games, we will have many experiences for
        each of those 10 games, but len(values) == 10.
        """
        assert(len(values) == self.buffer_size)

        exp_states = []
        exp_values = []
        exp_policies = []
        exp_phases = []

        for i in range(self.buffer_size):
            # get relevant data for this buffer entry
            states = self.buffer_gs[i]
            policies = self.buffer_pol[i]
            phases = self.buffer_ph[i]
            teams = self.buffer_gs_teams[i]
            value = values[i]
            state_values = [value if team == 0 else -value
                            for state, team in zip(states, teams)]
            # note: above we are ensuring we write the value with respect
            # to the acting team in each corresponding state!

            assert(len(states) == len(policies))
            assert(len(states) == len(phases))
            assert(len(states) == len(teams))

            exp_states += states
            exp_values += state_values
            exp_policies += policies
            exp_phases += phases

            # Reset lists to reduce memory usage:
            self.buffer_gs[i] = []
            self.buffer_pol[i] = []
            self.buffer_ph[i] = []
            self.buffer_gs_teams[i] = []

        del self.buffer_gs
        del self.buffer_pol
        del self.buffer_ph
        del self.buffer_gs_teams

        # Convert numpy to string, so we can serialise it:
        def to_serialisable_format(A):
            # Convert A to numpy array first, then
            # reshape so that it's a 1D array, then
            # convert to list then string to get it
            # in a good format, then trim the opening
            # and closing brackets.
            return str(list(np.array(A).reshape((-1,))))[1:-1]
        exp_states = map(to_serialisable_format, exp_states)
        exp_policies = map(to_serialisable_format, exp_policies)
        exp_phases = map(to_serialisable_format, exp_phases)

        # Put data in a Pandas dataframe, embed this
        # in a Dask dataframe, save to new file.
        new_data = pd.DataFrame(data=[(s, v, po, ph) for
                                      s, v, po, ph in zip(
                                      exp_states,
                                      exp_values,
                                      exp_policies,
                                      exp_phases
                                      )],
                                columns=['state', 'value',
                                         'policy', 'phase'])
        new_data = dd.from_pandas(new_data, npartitions=1)

        # Determine filename to save to (by replacing
        # wildcard.)
        i = 1
        while os.path.exists(self.filename.replace('*', str(i))):
            i += 1

        # Save the data
        new_data.to_parquet(self.filename.replace('*', str(i)))

    def add_to_buffer(self, game_states, teams, phases, policies, ids=None):
        """
        Add a list of game states to our running buffer, and the corresponding
        policies to learn. 'Policies' should be in numerical form, and should
        be a 'desirable' policy - the network will train with this as a target.
        Note: we do not have to add to all games in the buffer at once. The
        'ids' parameter, if not none, specifies a list of indices which tells
        us what game each of the states came from. This feature will be needed
        when some games finish (because finished games cannot be added to the
        buffer).
        'phases' and 'teams' represent the phase vector and team number
        corresponding to each game state. This is required because the game
        states are given in vector form, which doesn't give any info about the
        current phase or acting team.
        """
        assert(len(game_states) == len(policies))
        assert(len(game_states) == len(phases))
        assert(len(game_states) == len(teams))
        assert(len(game_states) == self.buffer_size
               if ids is None else len(game_states) == len(ids))

        if ids is not None:
            assert(sorted(list(set(ids))) == sorted(ids))  # check distinctness

        # default is that all games have been included
        if ids is None:
            ids = list(range(self.buffer_size))

        for i, j in enumerate(ids):  # enumerate: <idx, value>
            self.buffer_gs[j].append(game_states[i])
            self.buffer_pol[j].append(policies[i])
            self.buffer_ph[j].append(phases[i])
            self.buffer_gs_teams[j].append(teams[i])
