import numpy as np
import torch
from dqn import pytorch_util as ptu
from dqn.replay_buffer import ReplayBuffer
import os


class Agent:

    def __init__(self, env, agent_params):
        self.env = env
        self.agent_params = agent_params

        np.random.seed(self.agent_params['seed'])
        torch.manual_seed(self.agent_params['seed'])
        ptu.init_gpu(
            use_gpu=self.agent_params['use_gpu'],
            gpu_id=self.agent_params['which_gpu']
        )

        self.input_dims = self.env.observation_space.shape[0]
        self.output_dims = self.env.action_space.get('vel').shape[0]
        
        print("Input dims: ", self.input_dims)
        print("Output dims: ", self.output_dims)
        
        self.hidden_layers = agent_params['hidden_layers']
        self.hidden_size = agent_params['hidden_size']
        self.learning_rate = agent_params['learning_rate']
        self.batch_size = agent_params['batch_size']
        self.chkpt_dir = agent_params['chkpt_dir']
        self.q_chkpt_file = os.path.join(self.chkpt_dir, 'q_net.pth')
        self.q_target_chkpt_file = os.path.join(
            self.chkpt_dir, 'q_target_net.pth')

        self.gamma = self.agent_params['discount_factor']
        self.action_space = self.env.action_space

        self.eps = self.agent_params['eps']
        self.eps_decay_rate = self.agent_params['eps_decay_rate']
        self.eps_min = self.agent_params['eps_min']
        self.eps_decay_steps = self.agent_params['eps_decay_steps']

        self.learning_iter = 0
        self.replace_steps = self.agent_params['replace_target_network_steps']

        self.replay_buffer = ReplayBuffer(
            self.agent_params['replay_buffer_size'], self.env.observation_space.shape)

        self.q_net = ptu.build_mlp(
            input_size=self.input_dims,
            output_size=self.output_dims,
            n_layers=self.hidden_layers,
            hidden_size=self.hidden_size
        )
        self.optimizer = torch.optim.Adam(
            self.q_net.parameters(), lr=self.learning_rate)
        self.loss_fn = torch.nn.MSELoss()
        self.q_target_net = ptu.build_mlp(
            input_size=self.input_dims,
            output_size=self.output_dims,
            n_layers=self.hidden_layers,
            hidden_size=self.hidden_size
        )

    def add_to_replay_buffer(self, obs, action, reward, next_obs, done):
        self.replay_buffer.store_transition(
            obs, action, reward, next_obs, done)

    def choose_action(self, observation):
        if np.random.random() < self.eps:
            # choose random action from action space
            action = self.env.action_space.sample()
        else:
            # convert observation to pytorch tensor
            observation = ptu.from_numpy(observation)
            # predict q-values for current state with policy network
            actions = self.q_net.forward(observation)
            action = self.env.action_space.sample()
            # choose action with highest q-value
            actionVel = torch.argmax(actions).item()
            action['vel'] = [actionVel]

        return action

    def maybe_replace_target_network(self):
        # check if learn step counter is equal to replace target network counter
        if self.learning_iter % self.replace_steps == 0:
            # load weights of policy network and feed them into target network
            self.q_target_net.load_state_dict(self.q_net.state_dict())

    def maybe_decrement_epsilon(self):
        # check if current epsilon is still greater than epsilon min
        if self.eps > self.eps_min:
            # decrement epsilon by epsilon decay
            self.eps = self.eps*(1-self.eps_decay_rate)
        else:
            # set epsilon to epsilon min
            self.eps = self.eps_min

    def sample_memory(self):
        # get batch of transitions from replay memory
        obs, actions, rewards, next_obs, dones = self.replay_buffer.sample_buffer(
            self.batch_size)
        # convert to pytorch tensors and send to device (necessary for pytorch)
        obs = ptu.from_numpy(obs)
        actions = ptu.from_numpy(actions)
        rewards = ptu.from_numpy(rewards)
        next_obs = ptu.from_numpy(next_obs)
        dones = ptu.from_numpy(dones)

        return obs, actions, rewards, next_obs, dones

    def learn(self):
        # do not learn until memory size if greater or equal to batch size
        if self.replay_buffer.mem_cntr < self.batch_size:
            return

        # set gradients to zero to do the parameter update correctly
        # PyTorch accumulates the gradients on subsequent backward passes
        self.optimizer.zero_grad()

        # replace target network
        self.maybe_replace_target_network()

        # create batch indices
        batch_index = np.arange(self.batch_size)

        # get batch for training
        obs, actions, rewards, next_obs, dones = self.sample_memory()

        # compute q_values for each state, based on the selected action - Shape [64, 1]
        q_eval = self.q_net.forward(obs)

        # compute q-values for each new_state with target network - Shape [64, 4]
        q_next = self.q_target_net.forward(next_obs)

        # set q_next values for terminal states equals zero (no future reward if episode terminals)
        dones = dones.type(torch.BoolTensor)
        q_next[dones] = 0.0

        # compute q-targets with reward, discount factor and best q-value for each action - Shape [64, 1]
        q_target = rewards + self.gamma * torch.max(q_next, dim=1)[0]

        # compute loss between q-targets and q-eval
        loss = self.loss_fn(q_target, q_eval).to(ptu.device)

        # compute gradients
        loss.backward()

        # perform optimization step (parameter update)
        self.optimizer.step()

        # decrement epsilon
        self.maybe_decrement_epsilon()

        # increase learn step counter
        self.learning_iter += 1

        return loss

    def save_checkpoint(self):
        torch.save(self.q_net.state_dict(), self.q_chkpt_file)
        torch.save(self.q_target_net.state_dict(), self.q_target_chkpt_file)

    def load_checkpoint(self):
        self.q_net.load_state_dict(torch.load(self.q_chkpt_file))
        self.q_target_net.load_state_dict(torch.load(self.q_target_chkpt_file))
