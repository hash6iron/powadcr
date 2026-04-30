import struct
import os

def read_le_word(data, offset):
    """Lee 2 bytes en formato little-endian"""
    return struct.unpack('<H', data[offset:offset+2])[0]

def read_le_dword(data, offset):
    """Lee 4 bytes en formato little-endian"""
    return struct.unpack('<I', data[offset:offset+4])[0]

def read_le_3bytes(data, offset):
    """Lee 3 bytes en formato little-endian"""
    return data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16)

def analyze_wav(filepath):
    print("=" * 70)
    print("ANÁLISIS DE WAV: xevious.wav")
    print("=" * 70)
    
    with open(filepath, 'rb') as f:
        data = f.read(1024)  # Leer primeros 1024 bytes
    
    print(f"Tamaño archivo: {os.path.getsize(filepath)} bytes")
    print(f"\nHeader RIFF:")
    print(f"  ChunkID: {data[0:4].decode('ascii')}")
    chunk_size = read_le_dword(data, 4)
    print(f"  ChunkSize: {chunk_size} bytes")
    print(f"  Format: {data[8:12].decode('ascii')}")
    
    # Subchunk1 (fmt)
    print(f"\nSubchunk1 (fmt):")
    print(f"  ID: {data[12:16].decode('ascii')}")
    subchunk1_size = read_le_dword(data, 16)
    print(f"  Tamaño: {subchunk1_size} bytes")
    
    audio_format = read_le_word(data, 20)
    num_channels = read_le_word(data, 22)
    sample_rate = read_le_dword(data, 24)
    byte_rate = read_le_dword(data, 28)
    block_align = read_le_word(data, 32)
    bits_per_sample = read_le_word(data, 34)
    
    print(f"  Audio Format: {audio_format} (1=PCM)")
    print(f"  Canales: {num_channels}")
    print(f"  Sample Rate: {sample_rate} Hz")
    print(f"  Byte Rate: {byte_rate} bytes/sec")
    print(f"  Block Align: {block_align} bytes")
    print(f"  Bits por muestra: {bits_per_sample}")
    
    # Buscar "data" chunk
    offset = 36
    while offset < 200:
        if data[offset:offset+4] == b'data':
            print(f"\nData Chunk encontrado en offset {offset}:")
            data_size = read_le_dword(data, offset + 4)
            print(f"  Tamaño data: {data_size} bytes")
            print(f"  Duración: {data_size / byte_rate:.2f} segundos")
            break
        offset += 1

def analyze_tzx(filepath):
    print("\n" + "=" * 70)
    print("ANÁLISIS DE TZX: xevious.tzx")
    print("=" * 70)
    
    with open(filepath, 'rb') as f:
        data = f.read()
    
    print(f"Tamaño archivo: {len(data)} bytes")
    
    # Header TZX
    print(f"\nHeader TZX:")
    print(f"  Signature: {data[0:7]}")
    print(f"  Major version: {data[7]}")
    print(f"  Minor version: {data[8]}")
    
    offset = 9
    block_num = 0
    
    print(f"\nBloques encontrados:")
    
    while offset < len(data) - 10 and block_num < 20:  # Limitar a 20 bloques
        block_id = data[offset]
        print(f"\n  Bloque #{block_num}: ID 0x{block_id:02X} en offset {offset}")
        
        if block_id == 0x10:  # Standard Speed Data Block
            pulse_len = read_le_word(data, offset + 1)
            pause = read_le_word(data, offset + 3)
            data_len = read_le_word(data, offset + 5)
            print(f"    Standard Speed Data: pulso={pulse_len} T-states, pausa={pause}ms, datos={data_len} bytes")
            offset += 7 + data_len
            
        elif block_id == 0x11:  # Turbo Speed Data Block
            pulse_len = read_le_word(data, offset + 1)
            sync1 = read_le_word(data, offset + 3)
            sync2 = read_le_word(data, offset + 5)
            bit0_len = read_le_word(data, offset + 7)
            bit1_len = read_le_word(data, offset + 9)
            pause = read_le_word(data, offset + 11)
            data_len = read_le_3bytes(data, offset + 13)
            print(f"    Turbo Speed Data: pulso={pulse_len}, sync1={sync1}, sync2={sync2}")
            print(f"    bit0={bit0_len}, bit1={bit1_len}, pausa={pause}ms, datos={data_len} bytes")
            offset += 16 + data_len
            
        elif block_id == 0x15:  # Direct Recording Block
            sample_rate = read_le_word(data, offset + 1)
            # Completar con el tercer byte si lo hay
            sample_rate_byte3 = data[offset + 3]
            pause = read_le_word(data, offset + 4)
            mask_bits = data[offset + 6]
            data_len = read_le_3bytes(data, offset + 7)
            print(f"    Direct Recording: SR={sample_rate}Hz (byte3={sample_rate_byte3}), pausa={pause}ms")
            print(f"    Máscara útil: {mask_bits} bits del último byte, datos={data_len} bytes")
            print(f"    Primeros bytes de datos: {' '.join(f'{b:02X}' for b in data[offset+10:min(offset+20, len(data))])}")
            offset += 10 + data_len
            
        elif block_id == 0x20:  # Pause or stop the tape
            pause = read_le_word(data, offset + 1)
            print(f"    Pause/Stop: {pause}ms")
            offset += 3
            
        elif block_id == 0x21:  # Group start
            length = data[offset + 1]
            group_name = data[offset+2:offset+2+length].decode('ascii', errors='ignore')
            print(f"    Group Start: {group_name}")
            offset += 2 + length
            
        elif block_id == 0x22:  # Group end
            print(f"    Group End")
            offset += 1
            
        elif block_id == 0x0A:  # Pulse Sequence
            num_pulses = read_le_word(data, offset + 1)
            print(f"    Pulse Sequence: {num_pulses} pulsos")
            # Cada pulso es 2 bytes
            pulse_data_size = num_pulses * 2
            pulses = []
            for i in range(min(10, num_pulses)):  # Mostrar primeros 10
                pulse = read_le_word(data, offset + 3 + i * 2)
                pulses.append(pulse)
            print(f"    Primeros pulsos: {pulses}")
            offset += 3 + pulse_data_size
            
        elif block_id == 0x14:  # Pure Data Block
            pulse_len = read_le_word(data, offset + 1)
            sync1 = read_le_word(data, offset + 3)
            sync2 = read_le_word(data, offset + 5)
            bit0_len = read_le_word(data, offset + 7)
            bit1_len = read_le_word(data, offset + 9)
            pause = read_le_word(data, offset + 11)
            data_len = read_le_3bytes(data, offset + 13)
            print(f"    Pure Data Block: bit0={bit0_len}, bit1={bit1_len}, pausa={pause}ms, datos={data_len} bytes")
            offset += 16 + data_len
            
        else:
            print(f"    Bloque ID 0x{block_id:02X} desconocido (offset {offset})")
            # Intentar saltar de forma inteligente
            if block_id >= 0x30 and block_id <= 0x3F:
                # Bloques con longitud de 3 bytes
                length = read_le_3bytes(data, offset + 1)
                print(f"    Intentando saltar {length} bytes")
                offset += 4 + length
            else:
                break
        
        block_num += 1

# Analizar ambos archivos
wav_file = r"c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\xevious.wav"
tzx_file = r"c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\xevious.tzx"

analyze_wav(wav_file)
analyze_tzx(tzx_file)
