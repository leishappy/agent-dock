"""AgentDock PC 端主程序

采集 AI Agent 状态，通过串口发送给 ESP32 显示。
"""
import serial
import json
import time

# TODO: 自动检测 COM 口
# TODO: 实现 agent_monitor 模块监听 Agent 日志

def main():
    print("AgentDock PC v0.1.0")
    # TODO: 串口连接 + 数据采集循环
    pass

if __name__ == "__main__":
    main()
