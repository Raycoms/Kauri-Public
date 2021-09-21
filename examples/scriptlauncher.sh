#!/bin/bash

for j in {1..4}
do
  for i in {1..1}
  do
        docker stack deploy -c hotstuff${j}.yaml hotstuff1 &
        sleep 440

        for container in $(docker ps -q -f name="server")
        do
                if [ ! $(docker exec -it $container bash -c "cd Kauri && test -e log0") ]
                then
                  docker exec -it $container bash -c "cd Kauri && tac log* | grep -m1 'commit <block'"
                  docker exec -it $container bash -c "cd Kauri && tac log* | grep -m1 'x now state'"
                  docker exec -it $container bash -c "cd Kauri && tac log* | grep -m1 'Average'"
                  break
                fi
        done

        docker stack rm hotstuff1
        sleep 30

  done
done

todo, move the yaml file to be produced by a script, and add manually the values in it, so we can easily increase the pipelining and alter params if we want
I wonder if we can adjust the other values.