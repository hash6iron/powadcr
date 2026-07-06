#!/usr/bin/env python3
"""
Analizador completo de archivo TZX - Encuentra TODOS los bloques, especialmente GDB (0x19)
"""
import struct
import sys

def read_bytes(f, n):
    """Lee n bytes del archivo"""
    data = f.read(n)
    if len(data) < n:
        return None
    return data

def parse_tzx_file(filepath):
    """Analiza la estructura completa del TZX"""
    with open(filepath, 'rb') as f:
        # Signature y version
        sig = f.read(7)
        if sig != b'ZXTape!':
            print(f"ERROR: Invalid signature: {sig}")
            return
        
        eof = f.read(1)
        version_major = ord(f.read(1))
        version_minor = ord(f.read(1))
        
        print(f"TZX Header:")
        print(f"  Signature: {sig}")
        print(f"  Version: {version_major}.{version_minor}")
        print(f"  EOF: {hex(ord(eof))}")
        print()
        
        block_count = 0
        gdb_blocks = []
        
        while True:
            offset = f.tell()
            block_id_byte = f.read(1)
            
            if not block_id_byte:
                print(f"End of file reached at offset 0x{offset:06x}")
                break
            
            block_id = ord(block_id_byte)
            
            if block_id == 0x10:  # Standard Speed Data
                pause_ms = struct.unpack('<H', f.read(2))[0]
                data_length = struct.unpack('<I', f.read(4))[0] & 0xFFFFFF  # 3 bytes
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Standard Speed Data (0x10)")
                print(f"  Pause: {pause_ms}ms")
                print(f"  Data length: {data_length} bytes")
                print(f"  Data starts at: 0x{f.tell():06x}")
                f.seek(offset + 1 + 2 + 3 + data_length)  # Skip data
                
            elif block_id == 0x19:  # Generalized Data Block (GDB)
                gdb_blocks.append((block_count, offset))
                block_len = struct.unpack('<I', f.read(4))[0]
                print(f"\n{'='*70}")
                print(f"BLOCK {block_count} @ 0x{offset:06x}: GENERALIZED DATA BLOCK (0x19) ⭐")
                print(f"  Block length: {block_len} bytes")
                print(f"  Full block ends at: 0x{offset + 1 + 4 + block_len:06x}")
                
                # Leer toda la estructura GDB
                gdb_data_start = f.tell()
                gdb_data = f.read(block_len)
                
                if len(gdb_data) >= 10:
                    # Parse GDB structure
                    totp = struct.unpack('<I', gdb_data[0:4])[0]  # Total pilot symbols
                    npp = gdb_data[4]  # Pulses per pilot
                    asp = gdb_data[5]  # Pilot definitions
                    totd = struct.unpack('<I', gdb_data[6:10])[0]  # Total data symbols
                    
                    print(f"  Total Pilot Symbols: {totp}")
                    print(f"  Pulses Per Pilot: {npp}")
                    print(f"  Pilot Symbol Definitions: {asp}")
                    print(f"  Total Data Symbols: {totd}")
                    print(f"  GDB Data starts at: 0x{gdb_data_start:06x}")
                    
                    # Calcular cuánto espacio ocupan pilotos vs datos
                    estimated_pilot_pulses = totp * npp
                    estimated_data_pulses = totd * 2  # Aproximado
                    print(f"  Estimated Pilot Pulses: {estimated_pilot_pulses}")
                    print(f"  Estimated Data Symbols: {totd}")
                    print(f"  Raw GDB block size: {block_len} bytes")
                print(f"{'='*70}\n")
                
            elif block_id == 0x20:  # Pause
                pause_ms = struct.unpack('<H', f.read(2))[0]
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Pause (0x20) - {pause_ms}ms")
                
            elif block_id == 0x32:  # Stop
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Stop Tape (0x32)")
                
            elif block_id == 0x11:  # Turbo Speed Data
                pause_ms = struct.unpack('<H', f.read(2))[0]
                data_length = struct.unpack('<I', f.read(4))[0] & 0xFFFFFF
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Turbo Speed Data (0x11)")
                print(f"  Data length: {data_length} bytes")
                f.seek(offset + 1 + 2 + 3 + data_length)
                
            elif block_id == 0x15:  # Direct Recording
                pause_ms = struct.unpack('<H', f.read(2))[0]
                num_samples = struct.unpack('<I', f.read(4))[0]
                bits_per_sample = ord(f.read(1))
                data_length = struct.unpack('<I', f.read(4))[0] & 0xFFFFFF
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Direct Recording (0x15)")
                print(f"  Samples: {num_samples}, Bits: {bits_per_sample}")
                print(f"  Data length: {data_length} bytes")
                f.seek(offset + 1 + 2 + 4 + 1 + 3 + data_length)
                
            elif block_id == 0xff:  # Extension
                block_len = struct.unpack('<H', f.read(2))[0]
                print(f"BLOCK {block_count} @ 0x{offset:06x}: Extension (0xff) - {block_len} bytes")
                f.seek(offset + 1 + 2 + block_len)
                
            else:
                print(f"BLOCK {block_count} @ 0x{offset:06x}: UNKNOWN (0x{block_id:02x})")
                break
            
            block_count += 1
        
        print(f"\n\nSUMMARY:")
        print(f"Total blocks: {block_count}")
        print(f"GDB blocks found: {len(gdb_blocks)}")
        for idx, offset in gdb_blocks:
            print(f"  - Block {idx} at 0x{offset:06x}")

if __name__ == "__main__":
    filepath = r"C:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Basil the Great Mouse Detective.tzx"
    parse_tzx_file(filepath)
