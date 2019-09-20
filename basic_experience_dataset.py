import numpy as np


class BasicExperienceDataset:
    """
    This class is used for recording "experiences" for the AI to learn
    from, so they can be used multiple times. This is a 'basic' dataset
    in the sense that everything is stored in memory; a better version
    would read and write experiences from a file, because the dataset
    could potentially get very large.
    """

    def __init__(self, capacity):
        self.cap = capacity  # max size of experience dataset

        self.experience_gs = []  # game states
        self.experience_ph = []  # phases
        self.experience_v = []  # values
        self.experience_pol = []  # policies

        self.buffer_gs = []  # list of game states per buffer element
        self.buffer_ph = []  # list of phases as vectors per buffer element
        self.buffer_pol = []  # list of policies per buffer element
        self.buffer_gs_teams = []  # list: acting team for each game state

        self.buffer_size = 0  # number of games being fed into buffer

    def sample(self, n):
        # returns game_states (as arrays), phase vectors, values, policies

        # get random sample of experience indices
        idxs = np.random.choice(list(range(len(self.experience_gs))), size=n)

        # get the data corresponding to each of the generated indices:
        states = np.array(list(map(lambda i: self.experience_gs[i], idxs)))
        values = np.array(list(map(lambda i: self.experience_v[i], idxs)))
        policies = np.array(list(map(lambda i: self.experience_pol[i], idxs)))
        phases = np.array(list(map(lambda i: self.experience_ph[i], idxs)))

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
        for i in range(self.buffer_size):
            # get relevant data for this buffer entry
            states = self.buffer_gs[i]
            policies = self.buffer_pol[i]
            phases = self.buffer_ph[i]
            teams = self.buffer_gs_teams[i]
            value = values[i]

            assert(len(states) == len(policies))
            assert(len(states) == len(phases))
            assert(len(states) == len(teams))

            # save data for this buffer entry
            self.experience_gs += states
            self.experience_pol += policies
            self.experience_ph += phases
            self.experience_v += [value if team == 0 else -value
                                  for state, team in zip(states, teams)]
            # note: above we are ensuring we write the value with respect
            # to the acting team in each corresponding state!

        # Now trim experience dataset if it is too big:
        excess = max(0, len(self.experience_gs) - self.cap)
        if excess > 0:
            del self.experience_gs[:excess]
            del self.experience_pol[:excess]
            del self.experience_ph[:excess]
            del self.experience_v[:excess]

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
