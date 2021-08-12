trap 'exit 0' INT

sleep 2

crypto=$1 #bls or secp256k1
fanout=$2
pipedepth=$3
pipelatency=$4

service="server-$KOLLAPS_UUID"
service0="server0-$KOLLAPS_UUID"
service1="server1-$KOLLAPS_UUID"

cd libhotstuff && git pull && git submodule update --recursive --remote
if [ $crypto = "bls" ]; then
  git checkout bls-branch
else
  git checkout libsec-branch
fi

git pull && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED=ON -DHOTSTUFF_PROTO_LOG=ON && make

echo $(dig A $service +short | sort -u)
echo $(dig A $service0 +short | sort -u)
echo $(dig A $service1 +short | sort -u)

dig A $service0 +short | sort -u | sed -e 's/$/ 1/' >ips
dig A $service1 +short | sort -u | sed -e 's/$/ 1/' >> ips
dig A $service +short | sort -u | sed -e 's/$/ 1/' >> ips

sleep 10

python3 scripts/gen_conf.py --ips "ips" --crypto $crypto --fanout $fanout --pipedepth $pipedepth --pipelatency $pipelatency

sleep 10

echo "Starting Client: "

gdb -ex r -ex bt -ex q --args ./examples/hotstuff-client --idx 0 --iter -900 --max-async 900 > clientlog0 2>&1
