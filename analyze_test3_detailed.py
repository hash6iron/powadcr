#!/usr/bin/env python3
import struct
import os

def analyze_tzx_detailed(filename):
    print(f"\n{'='*70}")
    print(f"Análisis detallado: {os.path.basename(filename)}")
    print('='*70)
    
    try:
        filesize = os.path.getsize(filename)
        print(f"Tamaño: {filesize} bytes\n")
        
        with open(filename, 'rb') as f:
            # Verificar header
            header = f.read(10)
            if header[:7] != b'ZXTape!':
                print("❌ No es un archivo TZX válido")
                return
            
            print(f"✓ Header TZX válido")
            print(f"  Version: {header[7]}.{header[8]}\n")
            
            # Leer bloques
            block_num = 0
            offset = 10
            
            while offset < filesize:
                # Leer ID del bloque
                f.seek(offset)
                block_id_data = f.read(1)
                if not block_id_data:
                    break
                
                block_id = block_id_data[0]
                block_num += 1
                block_start = offset
                print(f"[BLOQUE {block_num}] Offset 0x{offset:06X} - ID 0x{block_id:02X}")
                
                try:
                    if block_id == 0x10:  # Standard Speed Data Block
                        pause = struct.unpack('<H', f.read(2))[0]
                        length = struct.unpack('<H', f.read(2))[0]
                        print(f"  Pause: {pause}ms, Data: {length} bytes")
                        offset += 5 + length
                        
                    elif block_id == 0x11:  # Turbo Speed Data Block
                        pilot_len = struct.unpack('<H', f.read(2))[0]
                        sync1 = struct.unpack('<H', f.read(2))[0]
                        sync2 = struct.unpack('<H', f.read(2))[0]
                        bit0 = struct.unpack('<H', f.read(2))[0]
                        bit1 = struct.unpack('<H', f.read(2))[0]
                        pilot_pulses = struct.unpack('<H', f.read(2))[0]
                        mask_last_byte = struct.unpack('B', f.read(1))[0]
                        pause = struct.unpack('<H', f.read(2))[0]
                        length_bytes = f.read(3)
                        length = length_bytes[0] | (length_bytes[1] << 8) | (length_bytes[2] << 16)
                        
                        print(f"  Pilot: {pilot_len}T, Sync: {sync1}/{sync2}T, Bit: {bit0}/{bit1}T")
                        print(f"  Pilot pulses: {pilot_pulses}, Pause: {pause}ms, Data: {length} bytes")
                        offset += 19 + length
                        
                    elif block_id == 0x15:  # Direct Recording Block
                        # Leer los 3 primeros bytes como little endian
                        sr_bytes_data = f.read(3)
                        sr = sr_bytes_data[0] | (sr_bytes_data[1] << 8) | (sr_bytes_data[2] << 16)
                        
                        pause = struct.unpack('<H', f.read(2))[0]
                        mask_last_byte = struct.unpack('B', f.read(1))[0]
                        
                        # Leer los 3 bytes de longitud de datos
                        length_bytes_data = f.read(3)
                        length = length_bytes_data[0] | (length_bytes_data[1] << 8) | (length_bytes_data[2] << 16)
                        
                        print(f"  Sampling rate: {sr} Hz (bytes: {sr_bytes_data.hex()})")
                        print(f"  Pause: {pause}ms")
                        print(f"  Mask: {mask_last_byte:08b}")
                        print(f"  Data length: {length} bytes (bytes: {length_bytes_data.hex()})")
                        offset += 10 + length
                        
                    elif block_id == 0x20:  # Pause or Stop the Tape
                        pause = struct.unpack('<H', f.read(2))[0]
                        print(f"  Pause: {pause}ms")
                        offset += 3
                        
                    else:
                        print(f"  ID desconocido - saltando...")
                        break
                        
                except struct.error as e:
                    print(f"  ❌ Error al leer bloque: {e}")
                    break
                    
                print()
                    
    except FileNotFoundError:
        print(f"❌ Archivo no encontrado: {filename}")

# Analizar test3 en detalle
test3 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test3.tzx'
if os.path.exists(test3):
    analyze_tzx_detailed(test3)
else:
    print(f"❌ No existe: {test3}")
