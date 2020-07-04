#!/bin/bash
killall hotstuff-app
rep=({0..3})
for i in "${rep[@]}"; do
    echo "starting replica $i"
    #valgrind --leak-check=full ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    #gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    ./examples/hotstuff-app --conf ./hotstuff-sec-bls${i}.conf > log${i} 2>&1 &
done
echo "All replicas started. Let's issue some commands to be replicated (in 5 sec)..."
sleep 5
echo "Start issuing commands and the leader will be killed in 5 seconds"
./examples/hotstuff-client --idx 0 --iter -1 --max-async 4

