# echo_server

## Description

echo_server is a server which can echo back any message you sent to it.

To improve the performance, this project uses multithread programming to receive connections from clients.

I also wrote a pressure test program (pressure_test.py) to test the performance of this server.



## How to run ?

```shell=
make
./server <port>

# Open another terminal
nc localhost <port>
type anything and server will echo back same thing.
```

## Pressure Test

```shell=
# python version
python3 pressure_test.py

# CPP version
make
./pressure_test
```

## Performance

```shell=
python3 draw_chart.py
```

![performace](/performance.PNG)


## Reference

server.cpp was edited from [here](https://cppsecrets.com/users/2194110105107104105108981049711648504964103109971051084699111109/C00-Networking-Threaded-echo-server.php)



