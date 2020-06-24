#!/bin/bash

rep=({0..3})
if [[ $# -gt 0 ]]; then
  rep=($@)
fi

for j in {1..10}; do

  for i in "${rep[@]}"; do
    echo "starting replica $i"
    #valgrind --leak-check=full ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    #gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
    ./examples/hotstuff-app --conf ./hotstuff-sec-bls${i}.conf > log${i} 2>&1 &
  done

  echo "waiting...."
  sleep 60

  echo "shutting down and copying results"
  killall hotstuff-app
  grep " height=" log0 | tail -1 >> results

done
