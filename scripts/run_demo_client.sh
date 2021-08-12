#!/bin/bash
killall hotstuff-client

# Try to run the replicas as in run_demo.sh first and then run_demo_client.sh.
# Use Ctrl-C to terminate the proposing replica (e.g. replica 0). Leader
# rotation will be scheduled. Try to kill and run run_demo_client.sh again, new
# commands should still get through (be replicated) once the new leader becomes
# stable.

sleep 5

for j in {1..10}; do

  echo "starting clients"
  ./examples/hotstuff-client --idx $((j%10)) --iter -1 --max-async 4 &

  sleep 60
  killall hotstuff-client

done
