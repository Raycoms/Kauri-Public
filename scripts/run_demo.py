import os, re
import subprocess
import itertools
import argparse
import paramiko
import time

if __name__ == "__main__":
    ipSet = [l.strip() for l in open("ips", 'r').readlines()]
    ips = []

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    port = 22
    user = "root"
    password = ""

    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        ssh.connect(ipElSet[0],port,user,password,timeout = 10)
        command = "killall hostuff-app"
        ssh.exec_command(command)
        ssh.close()

        print(ipElSet)
        for x in range(int(ipElSet[1])):
            ips.append(ipElSet[0])

    prefix = 'hotstuff.gen'

    print(ips)

    i = 0
    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        ssh.connect(ipElSet[0],port,user,password,timeout = 10)
        for x in range(int(ipElSet[1])):
            command = "cd test/libhotstuff && ./examples/hotstuff-app --conf ./hotstuff.gen-sec{}.conf > log{} 2>&1 &".format(i, i)
            print(command)
            ssh.exec_command(command)
            i+=1

        ssh.close()


    time.sleep( 60 )

    print("Killing all processes again!")

    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        ssh.connect(ipElSet[0],port,user,password,timeout = 10)
        command = "killall hostuff-app"
        ssh.exec_command(command)
        ssh.close()

