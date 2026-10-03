import serial
import time
import os
import struct

# --- CONFIGURATION ---
SERIAL_PORT = '/dev/ttyACM0'  # Replace with your ESP32 port
BAUD_RATE = 2000000
SAVE_DIR = './gestures'

# Ensure the save directory exists
os.makedirs(SAVE_DIR, exist_ok=True)

try:
    print(f"Opening {SERIAL_PORT} at {BAUD_RATE} bps...")
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    
    # Disable DTR/RTS to prevent ESP32 from randomly resetting upon connection
    # ser.setDTR(False)
    # ser.setRTS(False)
    time.sleep(1) # Allow connection to stabilize

    print("Waiting for watch to enter 'USB Sync' mode...")
    
    # Send the 0xAA trigger byte until the ESP32 responds
    while True:
        ser.write(b'\xAA')
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        
        if line == "DONE":
            print("\n[SUCCESS] Sync Complete!")
            break
            
        if line.startswith("FILE:"):
            # Parse the metadata (FILE:filename.bin:7680)
            parts = line.split(':')
            filename = parts[1].lstrip('/') # Remove leading slash if present
            filesize = int(parts[2])
            
            print(f"\nReceiving {filename} ({filesize} bytes)...")
            
            # Send the ACK byte (0xBB) to tell the ESP32 to open the floodgates
            ser.write(b'\xBB')
            
            # Read the exact number of raw binary bytes requested
            binary_data = ser.read(filesize)
            
            if len(binary_data) == filesize:
                filepath = os.path.join(SAVE_DIR, filename)
                with open(filepath, 'wb') as f:
                    f.write(binary_data)
                print(f"Saved -> {filepath}")
            else:
                print(f"[ERROR] Expected {filesize} bytes, but received {len(binary_data)}.")

except serial.SerialException as e:
    print(f"Serial Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")