#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import argparse
import torch
from ns3gym import ns3env


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
simTime = 100 # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--duration": simTime}
debug = False

env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)
# simpler:
#env = ns3env.Ns3Env()
env.reset()

ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space,  ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)


# dqn parameters

input_size = 15
output_size = 1
batch_size = 32
epoch_num = 1000
learning_rate = 0.001

for epoch in range(epoch_num):
    obs = env.reset()
    done = False
    while not done:
        action = env.action_space.sample()
        obs, reward, done, info = env.step(action)
        print("Reward: ", reward)
        print("Info: ", info)
        print("Done: ", done)

    print("Epoch: ", epoch)
    