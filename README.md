## async-ping

## build and test
* how to compile:
	* ```cd src; make``` then check bin/xping

* memcheck command
    * valgrind --tool=memcheck --leak-check=full ./xping


## usage
``` bash
curl localhost:9201/net/ping --data '{"timeout": 10, "servers": ["1.1.1.1", "2.2.2.2"]}'
```
network configuration of the machine where async-ping is running should be optimized.  You can add the following content to /etc/sysctl.conf) and run ```sysctl -p```

```
net.ipv4.neigh.default.gc_thresh1 = 4096
net.ipv4.neigh.default.gc_thresh2 = 8192
net.ipv4.neigh.default.gc_thresh3 = 8192
net.ipv4.neigh.default.base_reachable_time = 86400
net.ipv4.neigh.default.gc_stale_time = 86400
```
