#!/usr/bin/env python3
import serial
import time
import os
import json

def main():
    uart_port = "COM5"     # Change to your COM port (e.g., COM4, COM5, etc.)
    baud_rate = 115200     # Change as needed
    packet_file = "packet" # File containing the packet data

    # Check if file exists
    if not os.path.exists(packet_file):
        print(f"Error: File '{packet_file}' not found.")
        return

    try:
        # Load JSON packet
        with open(packet_file, "r") as f:
            packet_json = json.load(f)

        # Open serial connection
        ser = serial.Serial(uart_port, baud_rate, timeout=0.5)
        print(f"Opened UART on {uart_port} at {baud_rate} baud.")

        while True:
            # Convert packet to string and send
            packet_str = json.dumps(packet_json) + "\n"
            ser.write(packet_str.encode())
            print(f"Sent packet: {packet_str.strip()}")  # print the packet

            # Read response
            response = ser.readline().decode(errors="ignore").strip()
            if response:
                print(f"Received: {response}")

                # If response is like "channel-1"
                if response.startswith("channel-"):
                    try:
                        channel_num = int(response.split("-")[1])
                        print(f"Updating packet ID to {channel_num}")
                        packet_json["ID"] = channel_num
                    except ValueError:
                        print("Invalid channel format received.")

            time.sleep(0.1)  # adjust sending frequency

    except serial.SerialException as e:
        print(f"Error: {e}")

    except KeyboardInterrupt:
        print("\nStopped by user.")

if __name__ == "__main__":
    main()
