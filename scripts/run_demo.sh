#!/bin/bash
while read line; do
  IFS=' ' read -ra arr <<< "$line"
  ssh root@"${arr[0]}" "killall hotstuff-app 2>&1" &
done < ips

echo "wait 10"
sleep 10

j=$((0))
while read line; do
  echo $line;
  IFS=' ' read -ra arr <<< "$line"

  for (( i = 1; i <= arr[1]; i++ ))
  do
    echo "ssh root@${arr[0]} cd test/libhotstuff && ./examples/hotstuff-app --conf ./hotstuff.gen-sec${j}.conf > log${j} 2>&1"
    ssh root@"${arr[0]}" "cd test/libhotstuff && ./examples/hotstuff-app --conf ./hotstuff.gen-sec${j}.conf > log${j} 2>&1" &
    j=$((j+1))
  done

done < ips

sleep 60

while read line; do
  IFS=' ' read -ra arr <<< "$line"
  ssh root@"${arr[0]}" "killall hotstuff-app 2>&1" &
done < ips