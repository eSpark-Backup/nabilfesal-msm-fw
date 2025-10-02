import serial
import json
import random
 
def generate_phase_data(phase_name: str):
    """Generate dummy data for one phase."""
    return {
        "general": {
            "line_voltage": round(random.uniform(228, 232), 2),
            "rms_voltage": round(random.uniform(227, 231), 2),
            "frequency": 50.0,
            "current": round(random.uniform(9, 12), 2)
        },
        "power": {
            "factor": round(random.uniform(0.90, 0.99), 2),
            "active": round(random.uniform(900, 1200), 2),
            "reactive": round(random.uniform(180, 220), 2),
            "apparent": round(random.uniform(950, 1250), 2)
        },
        "energy": {
            "active": {
                "positive": random.randint(4800, 5200),
                "negative": random.randint(10, 30)
            },
            "reactive": {
                "positive": random.randint(900, 1200),
                "negative": random.randint(3, 8)
            }
        }
    }
 
def generate_channel_data(channel_id: int):
    """Generate dummy data for a channel with phases A, B, C."""
    return {
        "ID": channel_id,
        "status": 1,
        "temperature": round(random.uniform(25, 35), 2),
        "data": {
            "phase_a": generate_phase_data("A"),
            "phase_b": generate_phase_data("B"),
            "phase_c": generate_phase_data("C"),
        }
    }
 
# Serial setup
PORT = "COM5"  # Use second PTY from socat
BAUD = 115200
 
ser = serial.Serial(PORT, BAUD, timeout=1)
print(f"Listening on {PORT}...")
 
try:
    while True:
        if ser.in_waiting:
            line = ser.readline().decode().strip()
            print(f"Received: {line}")
 
            # Parse the channel number (e.g. "channel-1")
            if line.startswith("channel-"):
                try:
                    ch_num = int(line.split("-")[1])
                    if 1 <= ch_num <= 12:
                        response = generate_channel_data(ch_num)
                    else:
                        response = {"error": "Invalid channel"}
                except ValueError:
                    response = {"error": "Invalid channel format"}
            else:
                response = {"error": "Unknown command"}
 
            json_data = json.dumps(response) + "\n"
            ser.write(json_data.encode())
            print(f"Sent: {json_data.strip()}")
 
except KeyboardInterrupt:
    ser.close()
    print("Closed serial port.")