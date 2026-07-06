#!/usr/bin/env python3
import struct

def analyze_all_blocks(filename):
    """List ALL blocks in order"""
    with open(filename, 'rb') as f:
        data = f.read()
    
    pos = 10  # Skip TZX header
    block_num = 0
    
    print(f"File size: {len(data)} bytes\n")
    print("BLOCK LISTING:")
    print("="*80)
    
    while pos < len(data) - 1:
        block_id = data[pos]
        
        print(f"\nBLOCK #{block_num} at offset 0x{pos:06x}")
        print(f"  ID: 0x{block_id:02x}", end="")
        
        # Decode block type
        if block_id == 0x10:
            print(" (Standard Speed Data)")
            pause = struct.unpack('<H', data[pos+1:pos+3])[0]
            length = struct.unpack('<H', data[pos+3:pos+5])[0]
            print(f"    Pause: {pause} ms, Data: {length} bytes")
            pos += 5 + length
            
        elif block_id == 0x11:
            print(" (Turbo Speed Data)")
            # Skip complex structure
            pause = struct.unpack('<H', data[pos+15:pos+17])[0]
            length = struct.unpack('<I', data[pos+17:pos+21])[0]
            print(f"    Pause: {pause} ms, Data: {length} bytes")
            pos += 21 + length
            
        elif block_id == 0x19:
            print(" (Generalized Data Block - GDB)")
            block_len = struct.unpack('<I', data[pos+1:pos+5])[0]
            print(f"    Block Length: {block_len} bytes")
            pos += 5 + block_len
            
        elif block_id == 0x15:
            print(" (Direct Recording)")
            samples = struct.unpack('<I', data[pos+1:pos+5])[0]
            data_len = (samples + 7) // 8
            print(f"    Samples: {samples}")
            pos += 19 + data_len
            
        elif block_id == 0x20:
            print(" (Silence)")
            pause = struct.unpack('<H', data[pos+1:pos+3])[0]
            print(f"    Duration: {pause} ms")
            pos += 3
            
        elif block_id == 0x32:
            print(" (Stop Tape)")
            pos += 1
            
        elif block_id == 0xff:
            print(" (Extension)")
            # Read length as DWORD
            if pos + 5 <= len(data):
                length = struct.unpack('<I', data[pos+1:pos+5])[0]
                print(f"    Length: {length} bytes")
                pos += 5 + length
            else:
                break
                
        else:
            print(f" (Unknown)")
            # Try to guess length
            if block_id >= 0x20 and pos + 5 <= len(data):
                try:
                    length = struct.unpack('<I', data[pos+1:pos+5])[0]
                    if length < 10000000:
                        print(f"    Length: {length} bytes")
                        pos += 5 + length
                    else:
                        print(f"    Length seems invalid, moving 1 byte")
                        pos += 1
                except:
                    pos += 1
            else:
                pos += 1
        
        block_num += 1
        
        if block_num > 50:
            print(f"\n... (stopped at block 50 for safety)")
            break
    
    print(f"\n\nTotal blocks found: {block_num}")

if __name__ == "__main__":
    analyze_all_blocks("Basil the Great Mouse Detective.tzx")
