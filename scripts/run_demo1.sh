#!/bin/bash
killall hotstuff-app

rep=({0..4})
if [[ $# -gt 0 ]]; then
    rep=($@)
fi

for i in "${rep[@]}"; do
  echo "starting replica $i"
  #valgrind --leak-check=full ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
  #gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf hotstuff-sec${i}.conf > log${i} 2>&1 &
  ./examples/hotstuff-app --conf ./hotstuff.gen-sec${i}.conf > log${i} 2>&1 &
done

echo "waiting...."
sleep 60

echo "shutting down and copying results"
killall hotstuff-app
grep " height=" log0 | tail -1 >> results