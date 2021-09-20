#!/bin/bash

trap "docker stack rm kauriservice" EXIT

FILENAME=kauri.yaml
ORIGINAL_STRING=thecmd

FILENAME2="experiments"
LINES=$(cat $FILENAME2 | grep "^[^#;]")

# Each LINE in the experiment file is one experimental setup
for LINE in $LINES
do

  echo '---------------------------------------------------------------'
  echo $LINE
  sed  "s/${ORIGINAL_STRING}/${LINE}/g" $FILENAME > kauri-temp.yaml

  for i in {1..5}
  do
        # Deploy experiment
        docker stack deploy -c kauri-temp.yaml kauriservice &
        # Docker startup time + 5*60s of experiment runtime
        sleep 450
        
        # Collect and print results.
        for container in $(docker ps -q -f name="server")
        do
                if [ ! $(docker exec -it $container bash -c "cd Kauri-Public && test -e log0") ]
                then
                  docker exec -it $container bash -c "cd Kauri-Public && tac log* | grep -m1 'commit <block'"
                  docker exec -it $container bash -c "cd Kauri-Public && tac log* | grep -m1 'x now state'"
                  docker exec -it $container bash -c "cd Kauri-Public && tac log* | grep -m1 'Average'"
                  break
                fi
        done

        docker stack rm kauriservice
        sleep 30

  done
done
