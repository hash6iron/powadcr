import struct

def read_le_word(data, offset):
    return struct.unpack('<H', data[offset:offset+2])[0]

def read_le_dword(data, offset):
    return struct.unpack('<I', data[offset:offset+4])[0]

def read_le_3bytes(data, offset):
    return data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16)

with open('xevious.tzx', 'rb') as f:
    tz_data = f.read()

print("ANÁLISIS COMPLETO DE xevious.tzx")
print("=" * 70)
print(f"Tamaño: {len(tz_data)} bytes")
print(f"Versión: {tz_data[8]}.{tz_data[9]:02d}")
print()

offset = 10  # Después de header
block_num = 0
total_pulses = 0

while offset < len(tz_data) - 5 and block_num < 100:
    block_id = tz_data[offset]
    print(f"Bloque #{block_num}: ID 0x{block_id:02X} en offset {offset}")
    
    if block_id == 0x11:  # Turbo Speed Data Block
        pulse_len = read_le_word(tz_data, offset + 1)
        sync1 = read_le_word(tz_data, offset + 3)
        sync2 = read_le_word(tz_data, offset + 5)
        bit0_len = read_le_word(tz_data, offset + 7)
        bit1_len = read_le_word(tz_data, offset + 9)
        pause = read_le_word(tz_data, offset + 11)
        data_len = read_le_3bytes(tz_data, offset + 13)
        print(f"  Turbo Speed: pilot_pulse={pulse_len}, sync1={sync1}, sync2={sync2}")
        print(f"  Bits: 0={bit0_len}, 1={bit1_len}, pausa={pause}ms, datos={data_len} bytes")
        offset += 16 + data_len
        
    elif block_id == 0x10:  # Standard Speed Data Block
        pulse_len = read_le_word(tz_data, offset + 1)
        pause = read_le_word(tz_data, offset + 3)
        data_len = read_le_word(tz_data, offset + 5)
        print(f"  Standard Speed: pulse={pulse_len}, pausa={pause}ms, datos={data_len} bytes")
        offset += 7 + data_len
        
    elif block_id == 0x20:  # Pause
        pause = read_le_word(tz_data, offset + 1)
        print(f"  Pause: {pause}ms")
        offset += 3
        
    elif block_id == 0x15:  # Direct Recording
        sr_low = read_le_word(tz_data, offset + 1)
        sr_high = tz_data[offset + 3]
        pause = read_le_word(tz_data, offset + 4)
        mask = tz_data[offset + 6]
        data_len = read_le_3bytes(tz_data, offset + 7)
        sr = sr_low | (sr_high << 16)
        print(f"  Direct Recording: SR={sr}Hz, mask={mask}, data={data_len} bytes, pause={pause}ms")
        offset += 10 + data_len
        
    elif block_id == 0x0A:  # Pulse Sequence
        num_pulses = read_le_word(tz_data, offset + 1)
        total_pulses += num_pulses
        print(f"  Pulse Sequence: {num_pulses} pulsos")
        # Mostrar primeros 10 pulsos
        first_pulses = []
        for i in range(min(10, num_pulses)):
            p = read_le_word(tz_data, offset + 3 + i*2)
            first_pulses.append(p)
        print(f"    Primeros: {first_pulses[:5]}...")
        offset += 3 + num_pulses * 2
        
    elif block_id == 0x21:  # Group start
        length = tz_data[offset + 1]
        name = tz_data[offset+2:offset+2+length].decode('ascii', errors='ignore')
        print(f"  Group start: '{name}'")
        offset += 2 + length
        
    elif block_id == 0x22:  # Group end
        print(f"  Group end")
        offset += 1
        
    else:
        print(f"  Bloque desconocido 0x{block_id:02X} en offset {offset}")
        break
    
    block_num += 1
    print()

print(f"Total de bloques: {block_num}")
print(f"Total de pulsos escaneados: {total_pulses}")
