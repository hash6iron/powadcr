import struct
import sys

def read_le_word(data, offset):
    if offset + 2 > len(data):
        return 0
    return struct.unpack('<H', data[offset:offset+2])[0]

def read_le_3bytes(data, offset):
    if offset + 3 > len(data):
        return 0
    return data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16)

with open('xevious.tzx', 'rb') as f:
    data = f.read()

print("ANÁLISIS DETALLADO xevious.tzx")
print("=" * 70)
print(f"Tamaño total: {len(data)} bytes")
print(f"Versión: {data[8]}.{data[9]:02d}")
print()

offset = 10  # Después del header (9 bytes)
block_num = 0
total_data_bytes = 0

while offset < len(data) - 5 and block_num < 50:
    if offset >= len(data):
        break
        
    block_id = data[offset]
    print(f"\nBloque #{block_num}: ID 0x{block_id:02X} en offset {offset} (0x{offset:04X})")
    
    if block_id == 0x11:  # Turbo Speed Data Block
        if offset + 18 > len(data):
            print("  ERROR: No hay suficientes datos")
            break
            
        pilot_len = read_le_word(data, offset + 1)
        sync_1 = read_le_word(data, offset + 3)
        sync_2 = read_le_word(data, offset + 5)
        bit_0 = read_le_word(data, offset + 7)
        bit_1 = read_le_word(data, offset + 9)
        pilot_repeats = read_le_word(data, offset + 11)
        mask = data[offset + 13]
        pause = read_le_word(data, offset + 14)
        data_len = read_le_3bytes(data, offset + 16)
        
        total_data_bytes += data_len
        
        print(f"  Turbo Speed Data Block")
        print(f"    Pilot: {pilot_len} T-states x {pilot_repeats} repeats")
        print(f"    Sync: {sync_1} / {sync_2} T-states")
        print(f"    Bits: 0={bit_0}, 1={bit_1} T-states")
        print(f"    Mask: {mask} bits (0x{mask:02X})")
        print(f"    Pause: {pause} ms")
        print(f"    Data: {data_len} bytes")
        
        # Mostrar primeros y últimos bytes
        data_offset = offset + 19
        if data_len > 0:
            print(f"    First 8 bytes: {' '.join(f'{data[data_offset+i]:02X}' for i in range(min(8, data_len)))}")
            if data_len > 8:
                print(f"    Last 8 bytes: {' '.join(f'{data[data_offset+data_len-8+i]:02X}' for i in range(8))}")
        
        offset += 19 + data_len
        
    elif block_id == 0x10:  # Standard Speed Data Block
        if offset + 5 > len(data):
            break
        pulse_len = read_le_word(data, offset + 1)
        pause = read_le_word(data, offset + 3)
        data_len = read_le_word(data, offset + 5)
        total_data_bytes += data_len
        print(f"  Standard Speed: pulse={pulse}, pause={pause}ms, data={data_len} bytes")
        offset += 7 + data_len
        
    elif block_id == 0x20:  # Pause
        pause = read_le_word(data, offset + 1)
        print(f"  Pause: {pause}ms")
        offset += 3
        
    elif block_id == 0x15:  # Direct Recording
        if offset + 9 > len(data):
            break
        sr_low = read_le_word(data, offset + 1)
        sr_high = data[offset + 3]
        pause = read_le_word(data, offset + 4)
        mask = data[offset + 6]
        data_len = read_le_3bytes(data, offset + 7)
        total_data_bytes += data_len
        sr = sr_low | (sr_high << 16)
        print(f"  Direct Recording: SR={sr}Hz, mask={mask}, data={data_len} bytes, pause={pause}ms")
        offset += 10 + data_len
        
    elif block_id == 0x0A:  # Pulse Sequence
        num_pulses = read_le_word(data, offset + 1)
        pulse_data_size = num_pulses * 2
        print(f"  Pulse Sequence: {num_pulses} pulsos ({pulse_data_size} bytes)")
        offset += 3 + pulse_data_size
        
    elif block_id == 0x21:  # Group start
        length = data[offset + 1]
        name = data[offset+2:offset+2+length].decode('ascii', errors='ignore')
        print(f"  Group start: '{name}'")
        offset += 2 + length
        
    elif block_id == 0x22:  # Group end
        print(f"  Group end")
        offset += 1
        
    elif block_id == 0x30:  # Text description
        length = data[offset + 1]
        text = data[offset+2:offset+2+length].decode('ascii', errors='ignore')
        print(f"  Text: '{text}'")
        offset += 2 + length
        
    else:
        print(f"  ID 0x{block_id:02X} - NO IMPLEMENTADO")
        # Intentar saltar si tiene estructura estándar
        if offset + 4 < len(data):
            next_byte = data[offset + 1]
            if block_id >= 0x30:  # Probable 4-byte length
                length = struct.unpack('<I', data[offset+1:offset+5])[0]
                if length < 1000000:
                    print(f"    Saltando {length} bytes")
                    offset += 5 + length
                else:
                    print(f"    Length {length} parece inválida")
                    break
            else:
                break
        else:
            break
    
    block_num += 1

print(f"\n{'=' * 70}")
print(f"Total bloques: {block_num}")
print(f"Total data bytes: {total_data_bytes}")
print(f"Expected size at 96kHz: {int(total_data_bytes * 37.5)} bytes")
print(f"Expected duration at 96kHz: {total_data_bytes / 96000:.2f} seconds")
