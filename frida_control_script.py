import frida

msgs = []

def on_message(message: str, data: bytes):
    # Might need json.loads(message) to convert into Python object
    msgs.append(message)

device = frida.get_local_device()
pid = device.spawn("./local")
session = device.attach(pid)

code = open("other_frida_script.js").read()
script = session.create_script(code)
script.on("message", on_message)
script.load()

device.resume()

while not session.is_detached:
    pass

# To manually detach (could use signal.signal(signal.SIGINT, ...)):
# session.detach()

for msg in msgs:
    print(msg)
