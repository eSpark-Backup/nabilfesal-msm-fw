import serial
import json

# Mapping commands to dummy JSON responses
channel_data = {
    "channel-1": {
        "ID": 1,
        "status": 1,
        "temperature": 26.72,
        "data": {
            "phase_a": {
                "general": {
                    "line_voltage": 0.03,
                    "rms_voltage": 0.01,
                    "frequency": 50.00,
                    "current": 0.00
                },
                "power": {
                    "factor": 1.00,
                    "active": 0.00,
                    "reactive": 0.00,
                    "apparent": 0.00
                },
                "threshold": {
                    "current": 0.00,
                    "voltage": 43.85,
                    "angle": 130.76
                }
            }
        }
    },
    "channel-5": {
        "ID": 5,
        "status": 1,
        "temperature": 30.72,
        "data": {
            "phase_a": {
                "general": {
                    "line_voltage": 220.03,
                    "rms_voltage": 0.01,
                    "frequency": 50.00,
                    "current": 0.00
                },
                "power": {
                    "factor": 1.00,
                    "active": 0.00,
                    "reactive": 0.00,
                    "apparent": 0.00
                },
                "threshold": {
                    "current": 0.00,
                    "voltage": 43.85,
                    "angle": 240.76
                }
            }
        }
    },
    # Add "channel-2", ..., "channel-12" if needed
}

PORT = "COM5"  # Use second PTY from socat
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)
print(f"Listening on {PORT}...")

try:
    while True:
        if ser.in_waiting:
            line = ser.readline().decode().strip()
            print(f"Received: {line}")
            response = channel_data.get(line, {})
            json_data = json.dumps(response) + "\n"
            ser.write(json_data.encode())
            print(f"Sent: {json_data.strip()}")

except KeyboardInterrupt:
    ser.close()
    print("Closed serial port.")
