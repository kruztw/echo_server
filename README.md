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
python3 pressure_test.py
```


