import serial
import json
import random
from time import sleep
import subprocess
import time
import os
import re

PORT = "/tmp/py_port"
QT_PORT = "/tmp/qt_port"
BAUD = 921600
ser = None
socat_process = None

def cleanup_virtual_ports():
    """Kill old socat processes using our virtual ports and remove symlinks."""
    # Kill socat processes with /tmp/py_port or /tmp/qt_port
    try:
        output = subprocess.check_output(["pgrep", "-f", "socat"]).decode().splitlines()
        for pid in output:
            subprocess.run(["kill", "-9", pid])
    except subprocess.CalledProcessError:
        # No socat running
        pass

    # Remove leftover symlinks
    for path in [PORT, QT_PORT]:
        if os.path.exists(path) or os.path.islink(path):
            os.unlink(path)

def start_virtual_serial():
    """Start socat and create /tmp/qt_port <-> /tmp/py_port"""
    global socat_process

    print("[INFO] Starting socat to create virtual ports...")
    socat_process = subprocess.Popen([
        "socat", "-d", "-d",
        f"pty,raw,echo=0,link={QT_PORT}",
        f"pty,raw,echo=0,link={PORT}"
    ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Wait until symlinks appear
    for _ in range(50):
        if os.path.exists(PORT) and os.path.exists(QT_PORT):
            print("[INFO] Virtual ports ready.")
            return
        time.sleep(0.1)
    raise RuntimeError("socat failed to create virtual ports.")

def open_serial():
    """Open pyserial connection, wait if necessary."""
    global ser
    while True:
        if os.path.exists(PORT):
            try:
                ser = serial.Serial(PORT, BAUD, timeout=1)
                print(f"[INFO] Serial port opened: {PORT}")
                return
            except serial.SerialException:
                print(f"[WARN] Waiting to open {PORT}...")
        else:
            print(f"[WARN] {PORT} does not exist yet...")
        time.sleep(0.1)


# -------------------------

def generate_hex_voltage():
    """Generate a random hex value for voltage."""
    return hex(random.randint(0, 0xFFFF))

def handle_v_calibration():
    #sleep(6)
    """Generate response for v-calibration command."""
    channels = []
    for ch_id in range(1, 13):
        channels.append({
            "ID": ch_id,
            "phase_a": generate_hex_voltage(),
            "phase_b": generate_hex_voltage(),
            "phase_c": generate_hex_voltage()
        })
    return {
        "command": "v-calibration",
        "voltage": {"channels": channels}
    }

def handle_i_calibration(ch_id):
    sleep(1)
    return {
        "command": "i-calibration",
        "current": {
            "ID": ch_id,
            "phase_a": generate_hex_voltage(),
            "phase_b": generate_hex_voltage(),
            "phase_c": generate_hex_voltage()
        }
    }

def handle_set_ct_ratio():
    #sleep(2)
    return {
        "command": "ct-ratio",
        "message": "CT-Ratio updated successfully"
    }

def handle_set_v_offset():
    #sleep(1)
    return {
        "command": "v_offset",
        "message": "V-Offset updated successfully"
    }

def handle_set_i_offset():
    #sleep(1)
    return {
        "command": "i_offset",
        "message": "I-Offset updated successfully"
    }

def handle_reboot_calibration(resp):
    #sleep(1)
    return {
        "command": "reboot-calibration",
        "message": f"{resp} updated successfully"
    }

def replace_all_values_with_channel(obj, channel):
    """Recursively replace all numeric values with the channel number, except 'status'."""
    if isinstance(obj, dict):
        new_dict = {}
        for k, v in obj.items():
            if k == "status":
                new_dict[k] = v  # do NOT modify
            else:
                new_dict[k] = replace_all_values_with_channel(v, channel)
        return new_dict

    elif isinstance(obj, list):
        return [replace_all_values_with_channel(v, channel) for v in obj]

    elif isinstance(obj, (int, float)):
        return channel

    else:
        return obj


def generate_phase_data(force_zero_current=False):
    """Generate dummy data for one phase."""
    current_value = 0 if force_zero_current else round(random.uniform(9, 12), 2)

    return {
        "general": {
            "line_voltage": round(random.uniform(228, 232), 2),
            "rms_voltage": round(random.uniform(227, 231), 2),
            "frequency": 50.0,
            "current": current_value
        },
        "power": {
            "factor": round(random.uniform(500, 900), 2),
            "active": round(random.uniform(500, 900), 2),
            "reactive": round(random.uniform(500, 900), 2),
            "apparent": round(random.uniform(500, 900), 2)
        },
        "energy": {
            "active": {"positive": 111, "negative": 110},
            "reactive": {"positive": 333, "negative": 332}
        }
    }

def generate_channel_data(channel_id: int):
    """Generate dummy data for a channel with phases A, B, C."""
    # channel 2â€“12 â†’ current = 0
    force_zero = channel_id != 1

    return {
        "ID": channel_id,
        "status": 1,
        "temperature": 33.33,
        "data": {
            "phase_a": generate_phase_data(force_zero),
            "phase_b": generate_phase_data(force_zero),
            "phase_c": generate_phase_data(force_zero),
        }
    }

if __name__ == "__main__":
    cleanup_virtual_ports()   # Kill old socat + remove symlinks
    start_virtual_serial()    # Start fresh socat
    open_serial()             # Connect pyserial

    print(f"[INFO] Listening on {PORT}...")

    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode().strip()
                if not line:
                    continue

                # print(f"[DEBUG] Raw line received: {line}")

                response = None
                cmd = None

                # Handle channel requests
                if line.startswith("channel-"):
                    try:
                        ch_num = int(line.split("-")[1])
                        if 1 <= ch_num <= 12:
                            response = generate_channel_data(ch_num)
                        else:
                            response = {"error": "Invalid channel number"}
                    except ValueError:
                        response = {"error": "Invalid channel format"}

                # Handle JSON commands
                elif "{" in line and "}" in line:
                    try:
                        json_part = line[line.find("{"):line.rfind("}") + 1]
                        cmd = json.loads(json_part)
                    except Exception as e:
                        print(f"[ERROR] Failed to parse JSON: {e}")
                        response = {"error": "Invalid JSON"}

                # Handle recognized commands
                if cmd:
                    command_name = cmd.get("command")
                    # print(f"[DEBUG] command_name received: {command_name} counter: {command_counter}")
                    if command_name == "v-calibration":
                        response = handle_v_calibration()
                    elif command_name == "i-calibration":
                        ch_id = cmd.get("ID")
                        response = handle_i_calibration(ch_id)
                        # print(f"[i-calibration: {ch_id}] command_name: {command_name} response: {response}")
                    elif command_name == "set-ct-ratio":
                        response = handle_set_ct_ratio()
                    elif command_name == "set-v-offset":
                        response = handle_set_v_offset()
                    elif command_name == "set-i-offset":
                        response = handle_set_i_offset()
                    elif command_name == "reboot-calibration":
                        resp = "voltage" if "voltage" in cmd else "current"
                        response = handle_reboot_calibration(resp)
                    else:
                        response = {"error": "Unknown command"}

                if response:
                    ser.write((json.dumps(response) + "\n").encode())

    except KeyboardInterrupt:
        ser.close()
        print("[INFO] Closed serial port.")
        if socat_process:
            socat_process.terminate()
            print("[INFO] socat stopped.")