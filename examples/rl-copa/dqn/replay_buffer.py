import numpy as np


class ReplayBuffer:
    def __init__(self, max_size, input_dims):
        self.mem_size = max_size
        self.mem_cntr = 0

        self.observation_memory = np.zeros((self.mem_size, *input_dims),
                                     dtype=np.float32)
        self.new_observation_memory = np.zeros((self.mem_size, *input_dims),
                                         dtype=np.float32)
        self.action_memory = np.zeros(self.mem_size, dtype=np.int32)
        self.reward_memory = np.zeros(self.mem_size, dtype=np.float32)
        self.terminal_memory = np.zeros(self.mem_size, dtype=np.bool)

    def store_transition(self, observation, action, reward, new_observation, done):
        # compute index of first available memory
        index = self.mem_cntr % self.mem_size
        # store transitions based on index
        self.observation_memory[index] = observation
        self.new_observation_memory[index] = new_observation
        self.reward_memory[index] = reward
        self.action_memory[index] = action
        self.terminal_memory[index] = done
        # increase memory counter
        self.mem_cntr += 1

    def sample_buffer(self, batch_size):
        # find the highest occupied position
        max_mem = min(self.mem_cntr, self.mem_size)
        # create batch of random integers (from zero to max_mem in size of batch_size)
        # replace=False means that do not resample used transitions)
        batch = np.random.choice(max_mem, batch_size, replace=False)
        observations = self.observation_memory[batch]
        new_observations = self.new_observation_memory[batch]
        rewards = self.reward_memory[batch]
        actions = self.action_memory[batch]
        terminal = self.terminal_memory[batch]

        return observations, actions, rewards, new_observations, terminal