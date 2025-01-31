import frida
from typing import Dict, Optional

# Use this script to follow subprocesses:
# https://github.com/frida/frida-python/blob/main/examples/child_gating.py

msgs = []

def on_message(message: Dict, data: Optional[bytes]):
    # Might need json.loads(message["payload"]) to convert into Python object
    msgs.append(message["payload"])

device = frida.get_local_device()
pid = device.spawn("./local")
session = device.attach(pid)

code = open("frida_instrumentation_script.js").read()
script = session.create_script(code)
script.on("message", on_message)
script.load()

device.resume(pid)

while not session.is_detached:
    pass

# To manually detach (could use signal.signal(signal.SIGINT, ...)):
# session.detach()

for msg in msgs:
    print(msg)
