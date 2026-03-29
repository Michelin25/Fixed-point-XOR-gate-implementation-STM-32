This project is part of my embedded systems portfolio. I trained a small XOR neural network in TensorFlow and Scikit-learn to find the weights and biases, then reimplemented the network in C as a generic layer with a user defined architecture. This floating-point version served as the benchmark. I chose a custom C implementation instead of LiteRT to get more control over memory use and make debugging easier on an embedded target. I then converted the full design to fixed-point integers to reduce flash use, lower power use, and increase speed. 
After optimization, power use fell by X and speed increased by X.
| Function      | Optimization | Execution Time | Flash (text) | RAM (data + BSS) |
|---------------|--------------|----------------|--------------|------------------|
| `nn_predict`  | `-O0`        | 275 us         | 27212 Bytes  | 856 Bytes        |
| `nn_qpredict` | `-O0`        | 20 us          | 8024 Bytes   | 488 Bytes        |
| `nn_qpredict` | `-O3`        | 2.12 us        | 6972 Bytes   | 488 Bytes        |

Compared with `nn_predict` at `-O0`, `nn_qpredict` at `-O0` is about 13.75x faster, uses about 70.5% less flash, and uses about 43.0% less RAM.  
Changing `nn_qpredict` from `-O0` to `-O3` reduces execution time from 20 us to 2.12 us, which is about a 9.43x speedup, while flash usage drops by about 13.1%.  
RAM stays the same at 488 Bytes between `-O0` and `-O3`, so the optimization mainly improves speed and slightly reduces code size.  

This project is based on coursework from![ENGS-62](https://engineering.dartmouth.edu/courses/engs62) at Dartmouth's ![Thayer School of Engineering](https://engineering.dartmouth.edu/).

