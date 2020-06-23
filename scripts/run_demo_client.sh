#!/bin/bash
killall hotstuff-client

# Try to run the replicas as in run_demo.sh first and then run_demo_client.sh.
# Use Ctrl-C to terminate the proposing replica (e.g. replica 0). Leader
# rotation will be scheduled. Try to kill and run run_demo_client.sh again, new
# commands should still get through (be replicated) once the new leader becomes
# stable.

sleep 5

echo "starting clients"

clients=({1..10})
for i in "${clients[@]}"; do
  echo $((i%4))
  ./examples/hotstuff-client --idx $((i%4)) --iter -1 --max-async 4 &
done