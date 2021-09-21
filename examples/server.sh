trap 'exit 0' INT

sleep 2

crypto=$1 #bls or secp256k1
fanout=$2
pipedepth=$3
pipelatency=$4
latency=$5
bandwidth=$6
blocksize=$7

service="server-$KOLLAPS_UUID"
service1="server1-$KOLLAPS_UUID"

cd libhotstuff && git pull && git submodule update --recursive --remote
if [ $crypto = "bls" ]; then
  git checkout bls-branch
else
  git checkout libsec-branch
fi

git pull && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON && make

sleep 60

service="server-$KOLLAPS_UUID"
service1="server1-$KOLLAPS_UUID"
echo $(dig A $service +short | sort -u)
echo $(dig A $service1 +short | sort -u)

id=0
i=0

for ip in $(dig A $service1 +short | sort -u)
do
  for myip in $(ifconfig -a | awk '$1 == "inet" {print $2}')
  do
    if [ ${ip} == ${myip} ]
    then
      id=${i}
      echo "This is: ${ip}"
    fi
  done
  ((i++))
done
for ip in $(dig A $service +short | sort -u)
do
  for myip in $(ifconfig -a | awk '$1 == "inet" {print $2}')
  do
    if [ ${ip} == ${myip} ]
    then
      id=${i}
      echo "This is: ${ip}"
    fi
  done
  ((i++))
done



dig A $service1 +short | sort -u | sed -e 's/$/ 1/' > ips
dig A $service +short | sort -u | sed -e 's/$/ 1/' >> ips

sleep 5

python3 scripts/gen_conf.py --ips "ips" --crypto $crypto --fanout $fanout --pipedepth $pipedepth --pipelatency $pipelatency --block-size $blocksize

sleep 5

echo "Starting Application: #${i}"

gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf ./hotstuff.gen-sec${id}.conf > log${id} 2>&1 &

sleep 25

sudo tc qdisc add dev eth0 root netem delay ${latency}ms limit 400000 rate ${bandwidth}mbit &

sleep 30

if [ ${id} == 0 ]; then
  gdb -ex r -ex bt -ex q --args ./examples/hotstuff-client --idx 0 --iter -900 --max-async 900 > clientlog0 2>&1 &
fi

sleep 300

killall hotstuff-client &
killall hotstuff-app &
