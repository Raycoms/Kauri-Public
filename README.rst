Kauri
-----------

Kauri is a BFT communication abstraction that leverages dissemination/aggregation trees for load balancing and scalability while avoiding the main limitations of previous tree-based solutions, namely, poor throughput due to additional round latency and the collaps eof the tree to a star even in runs with few faults
while at the same time avoiding the bottleneck of star based solutions.

Paper
=====

This repo includes the prototype implementation evaluated in our 
*Kauri: Scalable BFT Consensus with PipelinedTree-Based Dissemination and Aggregation* paper.

Which will be published and presented at SOSP (https://sosp2021.mpi-sws.org/cfp.html)

Features
========

Kauri extends the publicly available implementation of HotStuff (https://github.com/hot-stuff/libhotstuff) with the following additions:

- Tree Based Dissemination and Aggregation equally balancing the message propagation and processing load among the internal nodes in the tree.

- BLS Signatures: Through BLS signatures the bandwidth load of the system is reduced significantly and signatures may be aggregated at each internal node.

- Extra Pipelining: Additional pipelining allows to offset the inherent latency cost of trees, allowing the system to perform significantly better even in high latency settings.

Run Kauri
=========

Disclaimer: The project is a prototype that was developped for the submission to SOSP. As such, it not production ready and still a work in progress considering certain system conditions.

https://github.com/Raycoms/Kauri-Consensus shows a step per step guide on how to run Kauri and reproduce several of the results in the paper.

