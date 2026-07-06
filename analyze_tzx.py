#!/usr/bin/env python3
import struct
import sys

def read_byte(f):
    return struct.unpack('B', f.read(1))[0]

def read_word(f):
    return struct.unpack('<H', f.read(2))[0]

def read_dword(f):
    return struct.unpack('<I', f.read(4))[0]

def analyze_tzx(filename):
    with open(filename, 'rb') as f:
        # Verificar header TZX
        header = f.read(10)
        if header[:7] != b'ZXTape!':
            print("ERROR: No es un archivo TZX válido")
            return
        
        major, minor = header[7], header[8]
        print(f"TZX Version: {major}.{minor}")
        print(f"EOF marker: {header[9]:02x}\n")
        
        # Mostrar primeros bytes en hex para debug
        f.seek(0)
        first_bytes = f.read(100)
        print(f"First 100 bytes (hex):")
        for i in range(0, len(first_bytes), 16):
            hex_str = ' '.join(f'{b:02x}' for b in first_bytes[i:i+16])
            ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in first_bytes[i:i+16])
            print(f"  {i:04x}: {hex_str:<48} {ascii_str}")
        
        print("\n" + "="*80)
        print("STRUCTURE ANALYSIS")
        print("="*80)
        while True:
            pos = f.tell()
            byte = f.read(1)
            
            if not byte:
                print(f"\nFin de archivo en posición {pos}")
                break
            
            block_id = byte[0]
            
            print(f"\n{'='*80}")
            print(f"BLOQUE #{block_num} | ID: 0x{block_id:02x} ({block_id:3d}) | Offset: {pos}")
            print(f"{'='*80}")
            
            # STOP TAPE - 0x00
            if block_id == 0x00:
                print("Stop the Tape")
            
            # STANDARD SPEED DATA - 0x10
            elif block_id == 0x10:
                pause = read_word(f)
                length = read_word(f)
                print(f"Standard Speed Data Block")
                print(f"  Pause after: {pause} ms")
                print(f"  Data length: {length} bytes")
                f.seek(pos + 5 + length)
            
            # TURBO SPEED DATA - 0x11
            elif block_id == 0x11:
                lead_pulse = read_word(f)
                bit0_pulse = read_word(f)
                bit1_pulse = read_word(f)
                pilot_len = read_word(f)
                last_bit = read_byte(f)
                pause = read_word(f)
                length = read_dword(f)
                print(f"Turbo Speed Data Block")
                print(f"  Leader pulse: {lead_pulse} T-states")
                print(f"  Bit-0 pulse: {bit0_pulse} T-states")
                print(f"  Bit-1 pulse: {bit1_pulse} T-states")
                print(f"  Pilot length: {pilot_len} pulses")
                print(f"  Last bit pulse: {last_bit}")
                print(f"  Pause after: {pause} ms")
                print(f"  Data length: {length} bytes")
                f.seek(pos + 19 + length)
            
            # GENERALIZED DATA BLOCK - 0x19
            elif block_id == 0x19:
                block_len = read_dword(f)
                pause = read_word(f)
                totp = read_dword(f)
                npp = read_byte(f)
                asp = read_byte(f)
                totd = read_dword(f)
                npd = read_byte(f)
                asd = read_byte(f)
                
                print(f"Generalized Data Block (GDB) - ID 0x19")
                print(f"  Block length: {block_len} bytes")
                print(f"  Pause after: {pause} ms")
                print(f"  PILOT/SYNC:")
                print(f"    TOTP (total pilot symbols): {totp}")
                print(f"    NPP (pulses per pilot symbol): {npp}")
                print(f"    ASP (pilot symbol definitions): {asp}")
                print(f"  DATA:")
                print(f"    TOTD (total data symbols): {totd}")
                print(f"    NPD (pulses per data symbol): {npd}")
                print(f"    ASD (data symbol definitions): {asd}")
                
                # Leer SYMDEF pilot
                print(f"\n  PILOT SYMBOL DEFINITIONS (ASP={asp}):")
                for i in range(asp):
                    symflag = read_byte(f)
                    polarity = symflag & 0x03
                    print(f"    Symbol {i}: Flag=0x{symflag:02x}, Polarity={polarity}", end="")
                    
                    # Polarity interpretations
                    pol_str = {0: "toggle", 1: "same", 2: "force-low", 3: "force-high"}
                    print(f" ({pol_str.get(polarity, '?')})")
                    
                    pulses = []
                    for j in range(npp):
                        pulse = read_word(f)
                        pulses.append(pulse)
                    print(f"      Pulses: {pulses}")
                
                # Leer PRLE pilot
                print(f"\n  PILOT STREAM (TOTP={totp}):")
                for i in range(totp):
                    symbol = read_byte(f)
                    repeat = read_word(f)
                    print(f"    Entry {i}: Symbol={symbol}, Repeat={repeat} times")
                
                # Leer SYMDEF data
                print(f"\n  DATA SYMBOL DEFINITIONS (ASD={asd}):")
                for i in range(asd):
                    symflag = read_byte(f)
                    polarity = symflag & 0x03
                    print(f"    Symbol {i}: Flag=0x{symflag:02x}, Polarity={polarity}", end="")
                    
                    pol_str = {0: "toggle", 1: "same", 2: "force-low", 3: "force-high"}
                    print(f" ({pol_str.get(polarity, '?')})")
                    
                    pulses = []
                    for j in range(npd):
                        pulse = read_word(f)
                        pulses.append(pulse)
                    print(f"      Pulses: {pulses}")
                
                # Calcular NB
                nb = 0
                temp = asd - 1
                while temp > 0:
                    nb += 1
                    temp >>= 1
                
                # Calcular DS
                ds = (nb * totd + 7) // 8
                print(f"\n  DATA STREAM:")
                print(f"    NB (bits per symbol): {nb}")
                print(f"    DS (stream bytes): {ds}")
                
                # Leer data stream (últimos DS bytes del bloque)
                current_pos = f.tell()
                block_end = pos + 1 + 4 + block_len
                data_offset = block_end - ds
                f.seek(data_offset)
                
                data_bytes = []
                for i in range(ds):
                    b = read_byte(f)
                    data_bytes.append(f"{b:02x}")
                
                print(f"    Data: {' '.join(data_bytes)}")
                
                f.seek(block_end)
            
            # PAUSE - 0x20
            elif block_id == 0x20:
                pause = read_word(f)
                print(f"Pause ({pause} ms)")
            
            # STOP THE TAPE - 0x32
            elif block_id == 0x32:
                print(f"Stop the Tape")
            
            # DIRECT RECORDING - 0x15
            elif block_id == 0x15:
                samples = read_dword(f)
                pause = read_word(f)
                bits_last = read_byte(f)
                sample_rate = read_word(f) | (read_byte(f) << 16)
                print(f"Direct Recording Block")
                print(f"  Samples: {samples}")
                print(f"  Pause after: {pause} ms")
                print(f"  Bits in last byte: {bits_last}")
                print(f"  Sample rate: {sample_rate} Hz")
                
                # Calculate data length
                data_len = (samples + 7) // 8
                f.seek(pos + 19 + data_len)
            
            else:
                # Skip unknown blocks
                # Try to read length if it follows standard format
                print(f"Unknown/Unhandled block type 0x{block_id:02x}")
                
                # Most blocks with ID >= 0x20 have a 4-byte length at offset +1
                try:
                    length = read_dword(f)
                    print(f"  Attempting to skip {length} bytes of data")
                    f.seek(pos + 5 + length)
                except:
                    print(f"  Could not skip - breaking")
                    break
            
            block_num += 1
            
            if block_num > 20:  # Limit para debug
                print(f"\n... (limited to first 20 blocks)")
                break

if __name__ == "__main__":
    filename = "Basil the Great Mouse Detective.tzx"
    analyze_tzx(filename)
