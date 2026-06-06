"""
AgentDock PC Agent - Claude Code state monitor (JSONL mode)
Usage: python agentd.py [esp32_host] [project_dir]

Principle:
  Claude Code writes every session event to ~/.claude/projects/*.jsonl
  We tail this file and parse events to determine state. Zero token overhead.

Events -> States:
  thinking block  -> THINKING
  tool_use block  -> TOOL (+ tool name)
  text block      -> RESPONDING
  tool_result     -> (back to thinking/responding)
  no active log   -> IDLE
"""
import asyncio
import json
import os
import sys
import time
import glob
from pathlib import Path

try:
    import websockets
except ImportError:
    print("Install dependency: pip install websockets")
    sys.exit(1)


# ---- JSONL log file discovery ----------------------------------

def get_claude_dir():
    return Path.home() / ".claude" / "projects"


def find_latest_jsonl(project_dir=None):
    base = get_claude_dir()
    if not base.exists():
        return None

    all_jsonl = []
    for f in base.rglob("*.jsonl"):
        if f.stat().st_size > 0:
            all_jsonl.append(f)

    if not all_jsonl:
        return None

    all_jsonl.sort(key=lambda f: os.path.getmtime(f), reverse=True)

    if project_dir:
        for f in all_jsonl:
            if project_dir.lower() in str(f).lower():
                return f

    return all_jsonl[0]


# ---- Event parser ----------------------------------------------

TOOL_LABELS = {
    "Bash": "Terminal", "Read": "Reading", "Write": "Writing",
    "Edit": "Editing", "Glob": "Searching", "Grep": "Searching",
    "Task": "Sub agent", "WebFetch": "Web fetch",
    "WebSearch": "Web search", "NotebookEdit": "Notebook",
}


def parse_event(line):
    try:
        evt = json.loads(line)
    except json.JSONDecodeError:
        return None

    msg = evt.get("message") or {}
    content = msg.get("content") or evt.get("content") or []
    if isinstance(content, dict):
        content = [content]
    if not isinstance(content, list):
        return None

    for block in content:
        if isinstance(block, str):
            continue
        btype = block.get("type", "")

        if btype == "thinking":
            return {"status": "thinking", "details": "Thinking"}

        if btype == "tool_use":
            name = block.get("name", "")
            detail = TOOL_LABELS.get(name, name or "Tool")
            return {"status": "tool", "details": detail}

        if btype == "text":
            return {"status": "responding", "details": "Generating"}

    if evt.get("type") == "system" and evt.get("subtype") == "init":
        return {"status": "responding", "details": "Session start"}

    return None


# ---- JSONL tailer ----------------------------------------------

class JSONLTailer:
    def __init__(self, path):
        self.path = path
        self.pos = 0

    def read_new_events(self):
        results = []
        if not self.path.exists():
            return results

        try:
            with open(self.path, "r", encoding="utf-8") as f:
                f.seek(0, 2)
                end = f.tell()
                if end < self.pos:
                    self.pos = 0
                if self.pos == end:
                    return results
                f.seek(self.pos)
                data = f.read(end - self.pos)
                self.pos = end

            for line in data.splitlines():
                line = line.strip()
                if not line:
                    continue
                try:
                    evt = parse_event(line)
                    if evt:
                        results.append(evt)
                except Exception as e:
                    print(f"[Warn] parse: {e} | {line[:100]}")
        except Exception:
            pass

        return results


# ---- WebSocket client ------------------------------------------

async def run(host="agentdock.local", port=80, project_dir=None):
    uri = f"ws://{host}:{port}/ws"
    print(f"AgentDock Agent (JSONL)")
    print(f"Target: {uri}")
    print(f"Log  : {get_claude_dir()}")
    print()

    prev_status = None
    reconnect_delay = 2
    last_hb = 0
    tailer = None
    last_file = None
    send_min_gap = 0.5   # min seconds between sends
    last_send = 0

    while True:
        try:
            async with websockets.connect(uri) as ws:
                print(f"Connected to {host}")
                reconnect_delay = 2

                try:
                    msg = await asyncio.wait_for(ws.recv(), timeout=5)
                    print(f"ESP32: {msg}")
                except asyncio.TimeoutError:
                    pass

                while True:
                    await asyncio.sleep(0.5)

                    # find / rotate log file
                    cur = find_latest_jsonl(project_dir)
                    if cur and cur != last_file:
                        print(f"[Log] Watching: {cur.name}")
                        tailer = JSONLTailer(cur)
                        last_file = cur

                    # read latest events
                    if tailer:
                        events = tailer.read_new_events()
                        if events:
                            evt = events[-1]  # only the last
                            now = time.time()
                            if evt["status"] != prev_status and (now - last_send) > send_min_gap:
                                prev_status = evt["status"]
                                last_send = now
                                payload = json.dumps({
                                    "t": "agent",
                                    "status": evt["status"],
                                    "details": evt["details"],
                                    "ts": int(now),
                                })
                                print(f"  -> SEND: {payload}")  # debug
                                await ws.send(payload)
                                if evt["status"] == "tool":
                                    await ws.send(json.dumps({
                                        "t": "notify",
                                        "title": "Tool Call",
                                        "body": evt["details"],
                                        "level": "info",
                                    }))
                                icon = {"idle": "-", "thinking": "T", "tool": ">", "responding": "R"}
                                print(f"[{icon.get(evt['status'], '?')}] {evt['status']:12s} {evt['details']}")

                    if not tailer and prev_status != "idle":
                        prev_status = "idle"
                        await ws.send(json.dumps({
                            "t": "agent", "status": "idle",
                            "details": "No active session", "ts": int(time.time()),
                        }))
                        print("[-] idle          No active session")

                    # heartbeat
                    if time.time() - last_hb > 10:
                        await ws.send(json.dumps({"t": "ping"}))
                        last_hb = time.time()

        except (websockets.ConnectionClosed, OSError) as e:
            print(f"Disconnected ({e}), retry in {reconnect_delay}s...")
            await asyncio.sleep(reconnect_delay)
            reconnect_delay = min(reconnect_delay * 1.5, 30)


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "agentdock.local"
    proj = sys.argv[2] if len(sys.argv) > 2 else None
    try:
        asyncio.run(run(host, project_dir=proj))
    except KeyboardInterrupt:
        print("\nStopped.")
