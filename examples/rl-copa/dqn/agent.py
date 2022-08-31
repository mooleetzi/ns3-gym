from typing_extensions import Self
import numpy as np
import torch
import pytorch_util as ptu
from replay_buffer import ReplayBuffer


class Agent:
    
    def __init__(self, env, agent_params):
        self.env = env
        self.agent_params = agent_params
        self.gamma = self.agent_params['gamma']
        self.standardize_advantages = self.agent_params['standardize_advantages']
        self.nn_baseline = self.agent_params['nn_baseline']
        self.reward_to_go = self.agent_params['reward_to_go']
        self.gae_lambda = self.agent_params['gae_lambda']
        
        self.replay_buffer = ReplayBuffer(self.agent_params['replay_buffer_size'], self.env.observation_space.shape)
        
        
        
        
        
        
    
        
        