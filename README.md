## Kauri


Kauri is a BFT communication abstraction that leverages dissemination/aggregation trees for load balancing and scalability while avoiding the main limitations of previous tree-based solutions, namely, poor throughput due to additional round latency and the collaps eof the tree to a star even in runs with few faults
while at the same time avoiding the bottleneck of star based solutions.

### Paper

This repo includes the prototype implementation evaluated in our 
*Kauri: Scalable BFT Consensus with PipelinedTree-Based Dissemination and Aggregation* paper.

Which will be published and presented at SOSP (https://sosp2021.mpi-sws.org/cfp.html)

### Features

Kauri extends the publicly available implementation of HotStuff (https://github.com/hot-stuff/libhotstuff) with the following additions:

- Tree Based Dissemination and Aggregation equally balancing the message propagation and processing load among the internal nodes in the tree.

- BLS Signatures: Through BLS signatures the bandwidth load of the system is reduced significantly and signatures may be aggregated at each internal node.

- Extra Pipelining: Additional pipelining allows to offset the inherent latency cost of trees, allowing the system to perform significantly better even in high latency settings.

### Run Kauri

Disclaimer: The project is a prototype that was developed for the submission to SOSP. As such, it not production ready and still a work in progress considering certain system conditions.
At the moment only bls signatures are supported. To run HotStuff with libsec signatures, this can be done by running vanilla Hotstuff at https://github.com/hot-stuff/libhotstuff.

#### Preliminary Setup

Building Kauri is very simple and only a couple of simple steps are necessary.
While Kauri can be run completely local on a single machine, we suggest running at most 20 processes per physical machine as depending on the configuration processes will start interfering with eachother (i.e 5 machines for 100 processes).

Make sure that Docker Version "20.10.5" or above is installed. Older Docker Versions won't work as they does not support adjusting network privilidges.

#### Docker Setup

First, checkout all the necessary code on all the host machines through

```
git clone https://github.com/Raycoms/Kauri-Public.git
cd Kauri-Public/runkauri
```

Build the Docker Images with:

```
docker build -t kauri .
```

On each of the physical machines.

Next, setup docker swarm with:

```
docker swarm init
```

On one of the servers.

Next, let the other machines join with:

```
docker swarm join --token <token> <ip>
```

Based on the token and IP the server where the init command was executed printed on start.

Next, promote all servers to manager through:

```
docker node promote <ip>
```

On the machine where "docker swarm init" was executed on.

On the same machine, setup a docker network with:

```
docker network create --driver=overlay --subnet=10.1.0.0/16 kauri_network
```

#### Run Experiments

To run and configure experiments we first take a look at the "experiments" file.

```
# type, fanout pipeline-depth pipeline-lat latency bandwidth
['bls','10','6','10','100','25']
# HotStuff has fanout = N
['bls','100','0','10','100','25']
```
Each of the lines represents an experiment, given a specific fanout, pipelining depth, latency and bandwidth.
By default, the number of nodes is 100.

To increase the number of nodes, enter the kauri.yaml file and adjust the number of replicas.
At the given moment, we seperated the replicas into two groups: Potential Internal Nodes and Leaf Nodes.
As such, for 100 nodes, considering a fanout of 10, there are 11 internal nodes (server1) and 89 remaining nodes (server).
This helps to balance internal nodes more equally over the different physical machines to reduce potential interference.

Note: At the given moment, only the 'bls' mode is supported.

Finally, to run the experiments, simply run:

```
./runexperiment.sh
```

This will run 5 instances of each of the setups defined in the "experiments" file.


#### Interpretation of Results

The above script will result in a regular output similar to:

```
2021-08-17 14:14:43.546142 [hotstuff proto] x now state: <hotstuff hqc=affd30ca8f hqc.height=2700 b_lock=22365a13f8 b_exec=63c209503b vheight=27xx tails=1>
```

Where 'hqc.height=2700' presents the last finalized block. Considering the 5 minute interval, that results in 2700/300 blocks per second.
Considering the default of 1000 transactions pr block, that results in `2700/300*1000 = 9000` ops per second.

