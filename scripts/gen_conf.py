import os, re
import subprocess
import itertools
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate configuration file for a batch of replicas')
    parser.add_argument('--prefix', type=str, default='hotstuff.gen')
    parser.add_argument('--ips', type=str, default=None)
    parser.add_argument('--iter', type=int, default=100)
    parser.add_argument('--pport', type=int, default=20000)
    parser.add_argument('--cport', type=int, default=30000)
    parser.add_argument('--keygen', type=str, default='./hotstuff-keygen')
    parser.add_argument('--tls-keygen', type=str, default='./hotstuff-tls-keygen')
    parser.add_argument('--nodes', type=str, default='nodes.txt')
    parser.add_argument('--block-size', type=int, default=1000)
    parser.add_argument('--pace-maker', type=str, default='dummy')
    parser.add_argument('--crypto', type=str, default='bls')
    parser.add_argument('--nworker', type=int, default=6)
    parser.add_argument('--fanout', type=int, default=10)
    parser.add_argument('--pipedepth', type=int, default=0)
    parser.add_argument('--pipelatency', type=int, default=10)

    args = parser.parse_args()


    if args.ips is None:
        ips = ['127.0.0.1']
    else:
        ipSet = [l.strip() for l in open(args.ips, 'r').readlines()]
        ips = []
        for ipEl in ipSet:
            ipElSet = ipEl.split(" ")
            print(ipElSet)
            for x in range(int(ipElSet[1])):
                ips.append(ipElSet[0])

    prefix = args.prefix
    iter = args.iter
    base_pport = args.pport
    base_cport = args.cport
    keygen_bin = args.keygen
    tls_keygen_bin = args.tls_keygen

    main_conf = open("{}.conf".format(prefix), 'w')
    nodes = open(args.nodes, 'w')

    i = 0
    replicas = []
    for ip in ips:
        replicas.append("{}:{};{}".format(ip, base_pport + i, base_cport + i))
        i+=1

    p = subprocess.Popen([keygen_bin, '--num', str(len(replicas)), '--algo', args.crypto],
                        stdout=subprocess.PIPE, stderr=open(os.devnull, 'w'))
    keys = [[t[4:] for t in l.decode('ascii').split()] for l in p.stdout]

    #tls_p = subprocess.Popen([tls_keygen_bin, '--num', str(len(replicas))], stdout=subprocess.PIPE, stderr=open(os.devnull, 'w'))
    #tls_keys = [[t[4:] for t in l.decode('ascii').split()] for l in tls_p.stdout]

    #f = open("tlskeys.txt", "w")
    #for keys in tls_keys:
    #    f.write(keys[0] + " " + keys[1] + " " + keys[2] + "\n")
    #f.close()

    tls_keys2 = [[n for n in line.strip().split(' ')] for line in open("tlskeys.txt", 'r').readlines()]

    if args.block_size is not None:
        main_conf.write("block-size = {}\n".format(args.block_size))
    if args.nworker is not None:
        main_conf.write("nworker = {}\n".format(args.nworker))
    if not (args.pace_maker is None):
        main_conf.write("pace-maker = {}\n".format(args.pace_maker))

    main_conf.write("proposer = {}\n".format(0))
    main_conf.write("fan-out = {}\n".format(args.fanout))
    main_conf.write("piped_latency = {}\n".format(args.pipelatency))
    main_conf.write("async_blocks = {}\n".format(args.pipedepth))

    for r in zip(replicas, keys, tls_keys2[:len(keys)], itertools.count(0)):
        main_conf.write("replica = {}, {}, {}\n".format(r[0], r[1][0], r[2][2]))
        r_conf_name = "{}-sec{}.conf".format(prefix, r[3])
        nodes.write("{}:{}\t{}\n".format(r[3], r[0], r_conf_name))
        r_conf = open(r_conf_name, 'w')
        r_conf.write("privkey = {}\n".format(r[1][1]))
        r_conf.write("tls-privkey = {}\n".format(r[2][1]))
        r_conf.write("tls-cert = {}\n".format(r[2][0]))
        r_conf.write("idx = {}\n".format(r[3]))
