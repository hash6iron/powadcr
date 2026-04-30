#!/usr/bin/env python3
import os

def hexdump(filename, num_bytes=200):
    try:
        print(f"\n{'='*70}")
        print(f"Hexdump: {os.path.basename(filename)}")
        print('='*70)
        
        with open(filename, 'rb') as f:
            data = f.read(num_bytes)
            
            # Print offset and hex values
            for i in range(0, len(data), 16):
                hex_part = ' '.join([f'{b:02X}' for b in data[i:i+16]])
                ascii_part = ''.join([chr(b) if 32 <= b < 127 else '.' for b in data[i:i+16]])
                print(f"{i:08X}: {hex_part:<48} {ascii_part}")
                
    except FileNotFoundError:
        print(f"❌ No existe: {filename}")

# Mostrar hexdump
test1 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test1.tzx'
test2 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test2.tzx'
test3 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test3.tzx'

for test in [test1, test2, test3]:
    if os.path.exists(test):
        # Get file size and show more bytes
        size = os.path.getsize(test)
        hex_bytes = min(300, size)
        hexdump(test, hex_bytes)
