#!/usr/bin/env python3

import asyncio
import time
import socket

connection_num = 10000
SERVER = "127.0.0.1"
PORT = 1234


async def request_server(msg: bytes) -> None:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((SERVER, PORT))
        s.send(msg)
        assert(s.recv(1024) == msg)    # make sure server echo back the correct messages.
    except Exception as e:
        print(f'error: {e}')
    finally:
        s.close()


async def main():
    tasks = []
    for i in range(connection_num):
        tasks.append(asyncio.create_task(request_server(('thread '+str(i)).encode())))

    print(f"started at {time.strftime('%X')}")
    for task in tasks:
        await task

    print(f"finished at {time.strftime('%X')}")


asyncio.run(main())
