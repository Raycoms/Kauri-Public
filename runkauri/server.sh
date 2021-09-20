trap 'exit 0' INT

sleep 2

# Initial Parameter Setup
crypto=$1
fanout=$2
pipedepth=$3
pipelatency=$4
latency=$5
bandwidth=$6
blocksize=$7

# Get Service-name
service="server-$KAURI_UUID"
service1="server1-$KAURI_UUID"

# Make sure correct branch is selected for crypto
cd Kauri-Public && git pull && git submodule update --recursive --remote
git checkout latest

# Do a quick compile of the branch
git pull && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON && make

sleep 30

id=0
i=0

# Go through the list of servers of the given services to identify the number of servers and the id of this server.


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

sleep 20

# Store all services in the list of IPs (first internal nodes then the leaf nodes)
dig A $service1 +short | sort -u | sed -e 's/$/ 1/' > ips
dig A $service +short | sort -u | sed -e 's/$/ 1/' >> ips

sleep 5

# Generate the HotStuff config file based on the given parameters
python3 scripts/gen_conf.py --ips "ips" --crypto $crypto --fanout $fanout --pipedepth $pipedepth --pipelatency $pipelatency --block-size $blocksize

sleep 20

echo "Starting Application: #${i}"

# Startup Kauri
gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf ./hotstuff.gen-sec${id}.conf > log${id} 2>&1 &

sleep 40

#Configure Network restrictions
sudo tc qdisc add dev eth0 root netem delay ${latency}ms limit 400000 rate ${bandwidth}mbit &

sleep 25

# Start Client on Host Machine
if [ ${id} == 0 ]; then
  gdb -ex r -ex bt -ex q --args ./examples/hotstuff-client --idx ${id} --iter -900 --max-async 900 > clientlog0 2>&1 &
fi

sleep 300

killall hotstuff-client &
killall hotstuff-app &

# Wait for the container to be manually killed
sleep 3000
