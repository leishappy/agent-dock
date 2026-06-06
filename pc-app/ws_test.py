"""
AgentDock — WebSocket 测试客户端
用法: python ws_test.py [host]
      host 默认 agentdock.local
"""
import asyncio
import sys
import json
import time
try:
    import websockets
except ImportError:
    print("先装依赖: pip install websockets")
    sys.exit(1)


async def test(host="agentdock.local", port=80):
    """连接 AgentDock 并发送测试数据"""
    uri = f"ws://{host}:{port}/ws"
    print(f"Connecting: {uri}")

    try:
        async with websockets.connect(uri) as ws:
            print("Connected!")

            # 收 hello
            msg = await ws.recv()
            print(f"← {msg}")

            # 发送任务列表
            tasks = {
                "t": "tasks",
                "list": [
                    {"id": "1", "name": "Compile firmware", "pct": 85, "status": "running"},
                    {"id": "2", "name": "Run unit tests",   "pct": 60, "status": "running"},
                    {"id": "3", "name": "Review PR #42",    "pct": 30, "status": "pending"},
                    {"id": "4", "name": "Deploy to staging","pct": 0,  "status": "pending"},
                ]
            }
            await ws.send(json.dumps(tasks))
            print(f"→ tasks (4 个)")

            time.sleep(2)

            # 发送天气
            weather = {"t": "weather", "temp": 26, "humidity": 65, "code": "sunny", "city": "Shenzhen"}
            await ws.send(json.dumps(weather))
            print(f"→ weather (Shenzhen 26°C)")

            time.sleep(2)

            # 发送通知
            notify = {"t": "notify", "title": "PR Merged!", "body": "feature/wifi merged to main", "level": "info"}
            await ws.send(json.dumps(notify))
            print(f"→ notify (PR Merged!)")

            time.sleep(2)

            # 切屏
            for i in range(3):
                await ws.send(json.dumps({"t": "screen", "id": i}))
                print(f"→ screen {i}")
                time.sleep(2)

            # ping
            await ws.send(json.dumps({"t": "ping"}))
            msg = await ws.recv()
            print(f"← {msg}")

            print("\nTest complete!")

    except Exception as e:
        print(f"Connection failed: {e}")
        print(f"Hint: 确保 ESP32 已启动并连接 WiFi")
        print(f"      检查 IP 或 mDNS: http://agentdock.local")


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "agentdock.local"
    asyncio.run(test(host))
