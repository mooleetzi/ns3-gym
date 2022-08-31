import numpy as np
import torch
from torch import nn
from typing import Union


Activation = Union[str, nn.Module]


_str_to_activation = {
    'relu': nn.ReLU(),
    'tanh': nn.Tanh(),
    'leaky_relu': nn.LeakyReLU(),
    'sigmoid': nn.Sigmoid(),
    'selu': nn.SELU(),
    'softplus': nn.Softplus(),
    'identity': nn.Identity(),
}


def build_mlp(input_size, output_size, n_layers, hidden_size, activation: Activation = 'tanh',
              output_activation: Activation = 'identity') -> nn.Module:
    if isinstance(activation, str):
        activation = _str_to_activation[activation]
    if isinstance(output_activation, str):
        output_activation = _str_to_activation[output_activation]
    layers = []
    # input layer
    layers.append(nn.Linear(input_size, hidden_size,device=device))
    layers.append(activation)
    # hidden layers
    layers.extend([
        nn.Linear(hidden_size, hidden_size,device=device),
        activation,
    ] * n_layers)
    # output layer
    layers.append(nn.Linear(hidden_size, output_size,device=device))
    layers.append(output_activation)

    return nn.Sequential(*layers)


device = None


def init_gpu(use_gpu=True, gpu_id=0):
    global device
    if torch.cuda.is_available() and use_gpu:
        device = torch.device("cuda:" + str(gpu_id))
        set_device(gpu_id)
        print("Using GPU id {}".format(gpu_id))
    else:
        device = torch.device("cpu")
        print("GPU not detected. Defaulting to CPU.")


def set_device(gpu_id):
    torch.cuda.set_device(gpu_id)


def from_numpy(*args, **kwargs):
    return torch.from_numpy(*args, **kwargs).float().to(device)


def to_numpy(tensor):
    return tensor.to('cpu').detach().numpy()
