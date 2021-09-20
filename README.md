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

### Preliminary Setup

Building Kauri is very simple and only a couple of simple steps are necessary.

The Experiments in the paper have been run on Grid5000 in the following setup:

```
1 Physical Machine per 20 processes
64GB per physical machine
16 cores per physical machine
Ubuntu 20 min with Git + docker installed
```
The default setup in this repository runs 100 nodes, as such, we suggest having at least 5 physical machines.

We suggest the above as a minimum setup, as starting up several containers and synchronizing them might take too long and time-out the system.
Also make sure that Docker Version "20.10.5" or above is installed. Older Docker Versions won't work as they does not support adjusting network privilidges.

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


On the main machine, setup a docker network with:

```
docker network create --driver=overlay --subnet=10.1.0.0/16 kauri_network
```

#### Run Experiments

To run and configure experiments we first take a look at the "experiments" file.

```
# type, fanout pipeline-depth pipeline-lat latency bandwidth :  number of internals : number of total : suggested physical machines
['bls','10','6','10','100','25','1000']:11:89:5
# HotStuff has fanout = N
['bls','100','0','10','100','25','1000']:11:89:5
```
Each of the lines represents an experiment, given a specific fanout, pipelining depth, latency and bandwidth and block-size.
Additionally, it includes the number of internal nodes + non internal nodes and finally the suggested number of physical servers.
(This will be printed in the log, and won't affect the overall execution)

The internal and non internal nodes are split to make sure docker swarm distributes the internal nodes more equally over the available physical machines.

By default, the number of nodes is 100.

To increase the number of nodes, enter the kauri.yaml file and adjust the number of replicas.
At the given moment, we seperated the replicas into two groups: Potential Internal Nodes and Leaf Nodes.
As such, for 100 nodes, considering a fanout of 10, there are 11 internal nodes (server1) and 89 remaining nodes (server).
This helps to balance internal nodes more equally over the different physical machines to reduce potential interference.

Note: To run HotStuff with libsec (vanilla), switch to the latest-libsec branch.

Finally, to run the experiments, simply run:

```
./runexperiment.sh
```

This will run 5 instances of each of the setups defined in the "experiments" file and reproduce the results of Figure 6c for Kauri and HotStuff-bls.
By adjusting the bandwidth and latency in the experiments file, the results for Figure 6a,6b and 7 may be obtained similarly.
To obtain the results of Figure 5, the pipelining may be adjusted.

#### Interpretation of Results

The above script will result in a regular output similar to:

```
2021-08-17 14:14:43.546142 [hotstuff proto] x now state: <hotstuff hqc=affd30ca8f hqc.height=2700 b_lock=22365a13f8 b_exec=63c209503b vheight=27xx tails=1>
2021-08-17 14:14:43.546145 [hotstuff proto] Average: 200"
```

Where 'hqc.height=2700' presents the last finalized block. Considering the 5 minute interval, that results in 2700/300 blocks per second.
Considering the default of 1000 transactions pr block, that results in `2700/300*1000 = 9000` ops per second.
The value next to "average" represents the average block latency.

#### Additional Experiments

When increasing the latency of the system, to make sure Kauri maintains its high throughput, the pipeline-depth has to be adjusted in the experiments file.
I.e for the experiment where the latency is varied between 50 and 400ms, alter the depth between 7,10,18 and 33 accordingly.

To obtain more detailed throughput data over the given execution time, the runexperiment.sh script has to be adjusted to export the entire log to pastebin similarly to:

```
cat log* | grep "proto" | grep "height" | pastebinit -b paste.ubuntu.com
```

In order to parse the time between the receival of a block and it's finalization.

To run the experiments including failures, exchange the branch in the Dockerfile to "reconfiguration" and re-run the docker build. Following that, adjust the server.sh script to launch one client per potential leader (By default only once client is launched that connects to process 0 to reduce the resource requirements). After this is done, one of the processes may be killed manually during execution time after 1 minute warmup.

Detailed throughput data may be extracted as explained above.

#### Troubleshooting

There are several factors that could prevent the system from executing propperly:
- Firewall settings between the physical machines
- Insufficient resources

Possible workarounds consist of:
- Reduceing the number of processes in kauri.yaml
- Increaseing the timeouts in server.sh and re-executing the build step.
