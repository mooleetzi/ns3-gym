#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import argparse
from ns3gym import ns3env
from dqn.agent import Agent
from dqn import util
import numpy as np


parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--iterations',
                    type=int,
                    default=1,
                    help='Number of iterations, Default: 1')
args = parser.parse_args()
startSim = bool(args.start)
iterationNum = int(args.iterations)

port = 5555
simTime = 100  # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--duration": simTime}
debug = False

env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim,
                    simSeed=seed, simArgs=simArgs, debug=debug)
# simpler:
#env = ns3env.Ns3Env()
env.reset()

config = util.load_config('./dqn/config/base.yaml')

agent = Agent(env, config)

ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space,  ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)

epoch_num = config['epoch_num']


# lists for storing data
episode_list = []
score_list = []
avg_score_list = []
epsilon_list = []
best_score = -np.inf

seed = config['seed']



for epoch in range(epoch_num):
    done = False
    score = 0
    observation = util.transform_state(env.reset())
    while not done:
        action = agent.choose_action(observation)
        observation_, reward, done, info = env.step(action)
        observation_ = util.transform_state(observation_)
        score += reward
        
        agent.add_to_replay_buffer(
            observation, action, reward, observation_, done)
        observation = observation_
        agent.learn()

    episode_list.append(epoch)
    score_list.append(score)
    epsilon_list.append(agent.eps)
    avg_score = np.mean(score_list[-100:])
    avg_score_list.append(avg_score)

    if avg_score > best_score:
        best_score = avg_score

agent.save_checkpoint()
util.plot_learning_curve(score_list, avg_score_list, config["env_name"])

# store training data and config to csv file
util.store_training_data(episode_list, score_list,
                         avg_score_list, epsilon_list, config["env_name"])
util.store_training_config(config, config["env_name"])
