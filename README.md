* how to compile
	* ```cd src; make``` then check bin/xping

* test via curl
    * curl localhost:9201/net/ping --data '{"timeout": 10, "servers": ["1.1.1.1", "2.2.2.2"]}'

* memcheck command
    * valgrind --tool=memcheck --leak-check=full ./xping

* network on machine should be configured (add following to /etc/sysctl.conf) and run ```sysctl -p```

```
net.ipv4.neigh.default.gc_thresh1 = 4096
net.ipv4.neigh.default.gc_thresh2 = 8192
net.ipv4.neigh.default.gc_thresh3 = 8192
net.ipv4.neigh.default.base_reachable_time = 86400
net.ipv4.neigh.default.gc_stale_time = 86400
```
