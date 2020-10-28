import os, re
import subprocess
import itertools
import argparse
import paramiko

if __name__ == "__main__":
    ipSet = [l.strip() for l in open("ips", 'r').readlines()]
    ips = []
    for ipEl in ipSet:
        ipElSet = ipEl.split(" ")
        print(ipElSet)
        for x in range(int(ipElSet[1])):
            ips.append(ipElSet[0])

    prefix = 'hotstuff.gen'

    print(ips)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    i = 0
    replicas = []
    for ip in ips:
        port = 22
        user = "root"
        password = ""
        # Establish a connection
        ssh.connect(ip,port,user,password,timeout = 10)
        command = "cd test/libhotstuff && ./examples/hotstuff-app --conf ./hotstuff.gen-sec{}.conf > log{} &".format(i, i)
        # Enter the Linux command
        stdin,stdout,stderr = ssh.exec_command(command)
        # Output command execution results
        result = stdout.read()
        print("finished!")
        print(result)
        ssh.close()
        i+=1

    #p = subprocess.Popen([keygen_bin, '--num', str(len(replicas)), '--algo', args.algo],stdout=subprocess.PIPE, stderr=open(os.devnull, 'w'))

