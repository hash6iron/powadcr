#!/usr/bin/env python3
import struct
import sys

def analyze_gdb_blocks(filename):
    """Find and analyze all GDB (Generalized Data Block) blocks"""
    with open(filename, 'rb') as f:
        data = f.read()
    
    # Look for GDB blocks (0x19)
    block_count = 0
    gdb_count = 0
    pos = 10  # Skip TZX header
    
    print(f"File size: {len(data)} bytes")
    print(f"Searching for blocks starting at offset 10...\n")
    
    while pos < len(data) - 5:
        block_id = data[pos]
        
        # GDB block (0x19)
        if block_id == 0x19:
            gdb_count += 1
            print(f"\n{'='*80}")
            print(f"GDB BLOCK #{gdb_count} at offset 0x{pos:06x} ({pos})")
            print(f"{'='*80}")
            
            # Parse GDB structure
            if pos + 19 > len(data):
                print("ERROR: Not enough data for GDB header")
                break
            
            block_len = struct.unpack('<I', data[pos+1:pos+5])[0]
            pause = struct.unpack('<H', data[pos+5:pos+7])[0]
            totp = struct.unpack('<I', data[pos+7:pos+11])[0]
            npp = data[pos+11]
            asp = data[pos+12]
            totd = struct.unpack('<I', data[pos+13:pos+17])[0]
            npd = data[pos+17]
            asd = data[pos+18]
            
            print(f"  Block Length: {block_len} bytes")
            print(f"  Pause After: {pause} ms")
            print(f"  PILOT/SYNC:")
            print(f"    TOTP (total pilot symbols): {totp}")
            print(f"    NPP (pulses per symbol): {npp}")
            print(f"    ASP (symbol definitions): {asp}")
            print(f"  DATA:")
            print(f"    TOTD (total data symbols): {totd}")
            print(f"    NPD (pulses per symbol): {npd}")
            print(f"    ASD (symbol definitions): {asd}")
            
            # Calculate NB and DS
            nb = 0
            temp = asd - 1
            while temp > 0:
                nb += 1
                temp >>= 1
            
            ds = (nb * totd + 7) // 8
            print(f"\n  DATA STREAM CALCULATION:")
            print(f"    NB (bits per symbol): {nb}")
            print(f"    DS (stream bytes): {ds}")
            
            # Parse symbol definitions
            offset = pos + 19
            
            # Pilot symbols
            print(f"\n  PILOT SYMBOL DEFINITIONS ({asp} symbols):")
            pilot_syms = []
            for i in range(asp):
                if offset >= len(data):
                    print(f"    ERROR: Cannot read symbol {i}")
                    break
                
                flag = data[offset]
                polarity = flag & 0x03
                offset += 1
                
                pulses = []
                for j in range(npp):
                    if offset + 1 >= len(data):
                        print(f"    ERROR: Cannot read pulse {j} of symbol {i}")
                        break
                    pulse = struct.unpack('<H', data[offset:offset+2])[0]
                    pulses.append(pulse)
                    offset += 2
                
                pol_str = {0: "toggle", 1: "same", 2: "force-low", 3: "force-high"}
                print(f"    [{i}] Flag=0x{flag:02x}, Polarity={pol_str.get(polarity, '?')} - Pulses: {pulses}")
                pilot_syms.append((polarity, pulses))
            
            # Pilot stream
            print(f"\n  PILOT STREAM ({totp} entries):")
            for i in range(totp):
                if offset + 2 >= len(data):
                    print(f"    ERROR: Cannot read entry {i}")
                    break
                symbol = data[offset]
                repeat = struct.unpack('<H', data[offset+1:offset+3])[0]
                offset += 3
                print(f"    [{i}] Symbol={symbol}, Repeat={repeat}")
            
            # Data symbols
            print(f"\n  DATA SYMBOL DEFINITIONS ({asd} symbols):")
            data_syms = []
            for i in range(asd):
                if offset >= len(data):
                    print(f"    ERROR: Cannot read symbol {i}")
                    break
                
                flag = data[offset]
                polarity = flag & 0x03
                offset += 1
                
                pulses = []
                for j in range(npd):
                    if offset + 1 >= len(data):
                        print(f"    ERROR: Cannot read pulse {j} of symbol {i}")
                        break
                    pulse = struct.unpack('<H', data[offset:offset+2])[0]
                    pulses.append(pulse)
                    offset += 2
                
                pol_str = {0: "toggle", 1: "same", 2: "force-low", 3: "force-high"}
                print(f"    [{i}] Flag=0x{flag:02x}, Polarity={pol_str.get(polarity, '?')} - Pulses: {pulses}")
                data_syms.append((polarity, pulses))
            
            # Data stream (últimos DS bytes del bloque)
            block_end = pos + 1 + 4 + block_len
            data_stream_offset = block_end - ds
            
            print(f"\n  DATA STREAM ({ds} bytes, starting at offset 0x{data_stream_offset:06x}):")
            if data_stream_offset >= len(data):
                print(f"    ERROR: Data stream offset out of bounds")
            else:
                data_bytes = data[data_stream_offset:block_end]
                hex_str = ' '.join(f'{b:02x}' for b in data_bytes)
                print(f"    {hex_str}")
                
                # Try to decode first few symbols
                print(f"\n  DATA STREAM DECODING (first {min(10, totd)} symbols):")
                bit_index = 0
                for sym_idx in range(min(10, totd)):
                    symbol_id = 0
                    for bit in range(nb):
                        if bit_index >= len(data_bytes) * 8:
                            break
                        byte_idx = bit_index // 8
                        bit_pos = 7 - (bit_index % 8)
                        bit_val = (data_bytes[byte_idx] >> bit_pos) & 1
                        symbol_id = (symbol_id << 1) | bit_val
                        bit_index += 1
                    
                    if symbol_id < len(data_syms):
                        pol, pulses = data_syms[symbol_id]
                        print(f"    Sym {sym_idx}: ID={symbol_id}, Polarity={pol}, Pulses={pulses}")
                    else:
                        print(f"    Sym {sym_idx}: ID={symbol_id} (OUT OF RANGE)")
            
            # Move to next block
            pos = block_end
            block_count += 1
            
        # Standard Speed Data (0x10)
        elif block_id == 0x10:
            print(f"\nStandard Speed Data at 0x{pos:06x}")
            pause = struct.unpack('<H', data[pos+1:pos+3])[0]
            length = struct.unpack('<H', data[pos+3:pos+5])[0]
            print(f"  Pause: {pause} ms, Length: {length} bytes")
            pos += 5 + length
            block_count += 1
        
        # Skip other known blocks
        elif block_id in [0x32]:  # Stop
            print(f"\nStop Tape at 0x{pos:06x}")
            pos += 1
            block_count += 1
        
        else:
            # Try to skip with 4-byte length
            if pos + 5 <= len(data) and block_id >= 0x20:
                try:
                    length = struct.unpack('<I', data[pos+1:pos+5])[0]
                    if length < 1000000:  # Sanity check
                        print(f"\nBlock 0x{block_id:02x} at 0x{pos:06x}, Length: {length}")
                        pos += 5 + length
                        block_count += 1
                    else:
                        pos += 1
                except:
                    pos += 1
            else:
                pos += 1
    
    print(f"\n\nSummary: Found {gdb_count} GDB blocks out of {block_count} total blocks")

if __name__ == "__main__":
    filename = "Basil the Great Mouse Detective.tzx"
    analyze_gdb_blocks(filename)
