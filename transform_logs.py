#!/usr/bin/env python3
"""
Script para transformar logs en PowaDCR a formato profesional con timestamp y clasificación.
Transforma logln(), log_v(), log_e(), log_i(), log_w(), log_d(), logHEX(), logBIN(), logAlert()
a formato: "[timestamp] [LEVEL] section - mensaje"
"""

import re
import os
from pathlib import Path
from datetime import datetime

# Rutas del proyecto
SRC_DIR = Path("src")
LIB_DIR = Path("lib")

# Clasificación automática de logs
LOG_LEVELS = {
    "ERROR": ["error", "failed", "fail", "not found", "invalid", "exception", "crash", "overflow", "underflow"],
    "ALERT": ["alert", "warning", "critical", "fatal", "panic"],
    "DEBUG": ["debug", "trace", "entering", "exiting", "step", "verbose"],
    "INFO": []  # Por defecto
}

def get_section_from_context(content, line_num):
    """Extrae el nombre de la función actual basado en el contexto."""
    lines = content[:line_num].split('\n')
    
    for i in range(len(lines) - 1, -1, -1):
        line = lines[i]
        # Buscar definición de función
        if re.search(r'^\s*(void|int|bool|String|char|double|float|uint\d+_t|File)\s+(\w+)\s*\(', line):
            match = re.search(r'^\s*(?:void|int|bool|String|char|double|float|uint\d+_t|File)\s+(\w+)\s*\(', line)
            if match:
                return match.group(1)
        # Buscar struct/class
        if re.search(r'^\s*(void|int|bool)\s+(\w+)::', line):
            match = re.search(r'^\s*(?:void|int|bool)\s+(\w+)::', line)
            if match:
                return match.group(1)
    
    return "MAIN"

def classify_log_level(message):
    """Clasifica el nivel del log basado en el contenido del mensaje."""
    msg_lower = message.lower()
    
    for level, keywords in LOG_LEVELS.items():
        if level == "INFO":
            continue
        for keyword in keywords:
            if keyword in msg_lower:
                return level
    
    return "INFO"

def transform_logln(match, section):
    """Transforma logln(...) al nuevo formato."""
    message = match.group(1)
    level = classify_log_level(message)
    
    if level == "ERROR":
        return f'log_error("{section}", {message})'
    elif level == "ALERT":
        return f'log_alert("{section}", {message})'
    elif level == "DEBUG":
        return f'log_debug("{section}", {message})'
    else:
        return f'log_info("{section}", {message})'

def transform_log_level_functions(content):
    """Transforma log_v(), log_e(), log_i(), log_w(), log_d()."""
    
    def replace_log_level(match):
        func_type = match.group(1)  # v, e, i, w, d
        message = match.group(2)
        section = "AUDIO"  # Estos suelen estar en log_v, log_i, etc.
        
        level_map = {
            'e': 'ERROR',
            'v': 'DEBUG',
            'i': 'INFO',
            'w': 'ALERT',
            'd': 'DEBUG'
        }
        
        level = level_map.get(func_type, 'INFO')
        func_map = {
            'e': 'log_error',
            'v': 'log_debug',
            'i': 'log_info',
            'w': 'log_alert',
            'd': 'log_debug'
        }
        
        func = func_map.get(func_type, 'log_info')
        return f'{func}(section, {message})'
    
    # Patrón: log_X(message)
    content = re.sub(r'log_([eviwdE])\s*\(\s*(".*?"|\'.*?\')\s*\)', replace_log_level, content)
    
    return content

def transform_file(filepath):
    """Transforma los logs en un archivo específico."""
    print(f"Procesando: {filepath}")
    
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    original_content = content
    
    # Obtener líneas para mapeo de secciones
    lines = content.split('\n')
    
    # Procesar cada línea
    new_lines = []
    for i, line in enumerate(lines):
        if 'logln(' in line or 'log_' in line:
            # Obtener sección del contexto
            section = get_section_from_context('\n'.join(lines[:i]), i)
            
            # Transformar logln()
            if 'logln(' in line:
                # Patrón simple: logln("mensaje")
                line = re.sub(
                    r'logln\s*\(\s*"([^"]*)"\s*\)',
                    lambda m: f'log_info("{section}", "{m.group(1)}")',
                    line
                )
                # Patrón con concatenación: logln("msg1" + var)
                line = re.sub(
                    r'logln\s*\(\s*"([^"]*)"(\s*\+.*?)\)',
                    lambda m: f'log_info("{section}", "{m.group(1)}"' + m.group(2) + ')',
                    line
                )
                # Patrón: logln(String o variable)
                line = re.sub(
                    r'logln\s*\(\s*([A-Za-z_]\w*)\s*\)',
                    lambda m: f'log_info("{section}", {m.group(1)})',
                    line
                )
        
        new_lines.append(line)
    
    content = '\n'.join(new_lines)
    
    # Solo guardar si hay cambios
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✓ Actualizado: {filepath}")
        return True
    
    return False

def main():
    print("=" * 70)
    print("TRANSFORMADOR DE LOGS - PowaDCR")
    print("=" * 70)
    print(f"Procesando archivos en: {SRC_DIR}, {LIB_DIR}")
    print()
    
    files_processed = 0
    files_changed = 0
    
    # Procesar .cpp y .h
    for pattern in ["**/*.cpp", "**/*.h"]:
        for filepath in SRC_DIR.glob(pattern):
            try:
                if transform_file(filepath):
                    files_changed += 1
                files_processed += 1
            except Exception as e:
                print(f"✗ Error en {filepath}: {e}")
    
    print()
    print("=" * 70)
    print(f"Archivos procesados: {files_processed}")
    print(f"Archivos modificados: {files_changed}")
    print("=" * 70)

if __name__ == "__main__":
    os.chdir(Path(__file__).parent)
    main()
