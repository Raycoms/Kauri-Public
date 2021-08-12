import os, re
import subprocess
import itertools
import argparse
import paramiko
import time
import os

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate configuration file for a batch of replicas')
    parser.add_argument('--latency', type=int, default=100)
    parser.add_argument('--bandwidth', type=int, default=100)
    args = parser.parse_args()

    ipSet = [l.strip() for l in open("ips", 'r').readlines()]
    ips = []

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    port = 22
    user = "root"
    password = ""
    latency = args.latency
    bandwidth = args.bandwidth

    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        if ipElSet[0] == "127.0.0.1":
            print("sys")
            os.system("killall hotstuff-app")
        else:
            ssh.connect(ipElSet[0],port,user,password,timeout = 10)
            ssh.exec_command("killall hotstuff-app")
            ssh.exec_command("sudo tc qdisc del dev enp5s0f0 root")
            ssh.close()

        print(ipElSet)
        for x in range(int(ipElSet[1])):
            ips.append(ipElSet[0])

    prefix = 'hotstuff.gen'
    time.sleep(3)
    print(ips)

    i = 0
    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        if ipElSet[0] == "127.0.0.1":
            for x in range(int(ipElSet[1])):
                command = "./examples/hotstuff-app --conf ./hotstuff.gen-sec{}.conf > log{} 2>&1 &".format(i, i)
                print(command)
                os.system(command)
                i+=1
        else:
            ssh.connect(ipElSet[0],port,user,password,timeout = 10)
            print("Run at {}".format(ipElSet[0]))
            for x in range(int(ipElSet[1])):
                command = "cd test/libhotstuff && gdb -ex r -ex bt -ex q --args ./examples/hotstuff-app --conf ./hotstuff.gen-sec{}.conf > log{} 2>&1 &".format(i, i)
                print(command)
                ssh.exec_command(command)
                i+=1

            ssh.close()

    time.sleep(3)

    i = 0
    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")
        if i == 0 :
            command = "sudo tc qdisc add dev enp5s0f0 root netem delay {}ms limit 400000 rate {}mbit".format(latency, bandwidth)
        else:
            command = "sudo tc qdisc add dev enp5s0f0 root netem delay {}ms limit 400000".format(latency)
        i+=1

        if ipElSet[0] != "127.0.0.1":
            ssh.connect(ipElSet[0],port,user,password,timeout = 10)
            print(command)
            ssh.exec_command(command)
            i+=1

    ssh.connect("172.16.52.2",port,user,password,timeout = 10)
    ssh.exec_command("killall hotstuff-client &")

    time.sleep(3)


    print("Starting Client!")
    ssh.exec_command("cd test/libhotstuff && ./examples/hotstuff-client --idx {} --iter -900 --max-async 900 > clientlog{} 2>&1 &".format(1, 1))

    time.sleep( 300 )

    ssh.exec_command("killall hotstuff-client &")
    print("Killing all processes again!")

    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")

        if ipElSet[0] == "127.0.0.1":
            print("sys")
            os.system("killall hotstuff-app")
        else:
            ssh.connect(ipElSet[0],port,user,password,timeout = 10)
            command = "killall hotstuff-app"
            ssh.exec_command(command)
            ssh.exec_command("sudo tc qdisc del dev enp5s0f0 root")
            ssh.close()
