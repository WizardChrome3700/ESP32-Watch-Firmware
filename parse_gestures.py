import os
import struct
import csv
import glob

# --- CONFIGURATION ---
INPUT_DIR = './gestures'
OUTPUT_DIR = './gestures_csv'

# The AdcFrame struct is exactly 36 bytes:
# - uint32_t timestamp (4 bytes)
# - int32_t channels[8] (32 bytes)
FRAME_SIZE = 36 

# Python struct format string:
# '<'  = Little-endian (ESP32 memory standard)
# 'I'  = Unsigned 32-bit integer (timestamp)
# '8i' = Eight signed 32-bit integers (ADC channels)
UNPACK_FORMAT = '<I8i' 

os.makedirs(OUTPUT_DIR, exist_ok=True)

# Find all .bin files in the target directory
bin_files = glob.glob(os.path.join(INPUT_DIR, '*.bin'))

if not bin_files:
    print(f"No .bin files found in {INPUT_DIR}.")

for bin_path in bin_files:
    filename = os.path.basename(bin_path)
    csv_filename = filename.replace('.bin', '.csv')
    csv_path = os.path.join(OUTPUT_DIR, csv_filename)
    
    with open(bin_path, 'rb') as f_in, open(csv_path, 'w', newline='') as f_out:
        writer = csv.writer(f_out)
        
        # Write the CSV header
        writer.writerow(['Timestamp_us', 'CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7'])
        
        frame_count = 0
        while True:
            chunk = f_in.read(FRAME_SIZE)
            
            # Stop if we reach the end of the file or a partial frame
            if len(chunk) < FRAME_SIZE:
                break
            
            # Unpack the 36 bytes directly into a tuple of 9 integers
            data = struct.unpack(UNPACK_FORMAT, chunk)
            writer.writerow(data)
            frame_count += 1
            
    print(f"Parsed {filename} -> {csv_filename} ({frame_count} frames)")

print("\n[SUCCESS] Parsing complete.")