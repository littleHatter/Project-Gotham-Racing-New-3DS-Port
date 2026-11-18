#!/usr/bin/env python3
"""Standalone Project Gotham Racing game-data preparation for New Nintendo 3DS.

Uses only the Python standard library. Bounded SIS parsing is inherited from
the compatibility project preparation tools.
GPL-2.0-or-later; see the project LICENSE. No game data is embedded here.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path, PurePosixPath
import re
import struct
import sys
import tempfile
import zlib

MAX_FILE = 32 * 1024 * 1024
MAX_PACKAGE = 128 * 1024 * 1024


class PreparationError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise PreparationError(message)


def fields(data, typed=True, element_type=None):
    """Read bounded, four-byte-aligned SIS fields, including extended lengths."""
    offset = 0
    while offset < len(data):
        require(len(data) - offset >= (8 if typed else 4), 'Truncated SIS field header.')
        kind = element_type
        if typed:
            kind = struct.unpack_from('<I', data, offset)[0]
            offset += 4
        size = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        if size & 0x80000000:
            require(len(data) - offset >= 4, 'Truncated extended SIS length.')
            size = (size & 0x7fffffff) | (struct.unpack_from('<I', data, offset)[0] << 31)
            offset += 4
        require(size <= len(data) - offset, 'SIS field exceeds its container.')
        yield kind, data[offset:offset + size]
        offset += (size + 3) & ~3
        require(offset <= len(data), 'Truncated SIS alignment padding.')


def array(data):
    require(len(data) >= 4, 'Truncated SIS array.')
    return list(fields(data[4:], False, struct.unpack_from('<I', data)[0]))


def first(data, kind):
    matches = [value for tag, value in fields(data) if tag == kind]
    require(len(matches) == 1, 'Missing or duplicate SIS field %d.' % kind)
    return matches[0]


def decompress(data):
    require(len(data) >= 12, 'Truncated SIS compression header.')
    algorithm, size = struct.unpack_from('<IQ', data)
    require(size <= MAX_FILE, 'SIS decoded file exceeds the supported size limit.')
    if algorithm == 0:
        result = data[12:]
    elif algorithm == 1:
        decoder = zlib.decompressobj()
        result = decoder.decompress(data[12:], size + 1)
        require(decoder.eof and not decoder.unconsumed_tail and not decoder.unused_data,
                'Incomplete or oversized SIS zlib stream.')
    else:
        raise PreparationError('Unsupported SIS compression algorithm %d.' % algorithm)
    require(len(result) == size, 'SIS decoded length mismatch.')
    return result


def descriptions(data, depth=0):
    require(depth <= 32, 'SIS controller nesting is too deep.')
    for kind, value in fields(data):
        if kind == 24:
            yield value
        elif kind in (13, 26, 27, 28):
            yield from descriptions(value, depth + 1)
        elif kind == 2:
            for element_kind, element in array(value):
                if element_kind == 24:
                    yield element
                elif element_kind in (26, 27, 28):
                    yield from descriptions(element, depth + 1)



SUPPORTED_PACKAGE_SHA256 = 'f19fbb3a01cd6869809ae8d9724d405026bfa9d7613a8ea5eb15bb5c697a8a04'
GAME_FILES = {'appdata.i3d': 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855', 'core_assets.dz': '48b75f0b030ac20b1fd7648040619b111803c206581670ac8d55ae1c7e663989', 'languageenglish.dz': '51197f5f1f057942577e99aee337b977620ce6eb6bd379778833734a88cb6459', 'languagefrench.dz': '88410a94d06f38e59a177ab961bcdfab1b7417c538c1d86b39da91d1c7026d99', 'languagegerman.dz': '587b9834a8fcd7e3a3eccfc6ba5b5f3d5d101817fd8148912cefd06fd5cf7d28', 'languageitalian.dz': '6ed5fe1153feff31739f1f17e6ffcfb61068c6ec7cd9ac8d4f050d8ed00f1b31', 'languageportugese-br.dz': '2fc3347a20090f98e15cc34499099c328e846cdf51b7513aee18b5d0744502c4', 'languageportugese.dz': '8c41bbe5851e780aefd05bcbfd8987a5583cdad3866082d432cb335d5989c919', 'languagespanish.dz': '5d35626a6b9a073deaa5186414738fba774490c051c9e190a1a496daf1f60593', 'pgr-splash.jpg': '4de00fa27695dfdebdc35897fdcfeb6857dfe781e636c0737ecdca6208b369a8', 'pgr.s3e': '6fffeafa4d7ff5c5fcf8636ee1ac65b00b7ebc62a77da13cae91c3e0a7b89e1b', 'pgr.xe3u': '7152f1e43c20c327b1e272af472a8a008962623aa44c7fbf28304faa8120ca0f'}


def prepare_data(package):
    package = Path(package)
    require(16 <= package.stat().st_size <= MAX_PACKAGE, 'Unsupported package size.')
    data = package.read_bytes()
    require(hashlib.sha256(data).hexdigest() == SUPPORTED_PACKAGE_SHA256,
            'This package is not the supported PGR edition. Expected SHA-256: ' + SUPPORTED_PACKAGE_SHA256)
    require(struct.unpack_from('<I', data)[0] == 0x10201a7a, 'Not a supported SIS/SISX file.')
    kind, contents = next(fields(data[16:]))
    require(kind == 12, 'Missing SIS contents.')
    controller = decompress(first(contents, 3))
    units = array(first(first(contents, 30), 2))
    require(len(units) == 1, 'Unsupported multi-unit package.')
    payloads = array(first(units[0][1], 2))
    installed = {}
    private = None
    for description in descriptions(controller):
        sequence = fields(description)
        tag, name_data = next(sequence)
        require(tag == 1, 'Missing installed filename.')
        name = name_data.decode('utf-16le').replace('\\', '/').lower()
        index = struct.unpack_from('<I', description, len(description) - 4)[0]
        require(index < len(payloads), 'Invalid package file index.')
        raw = decompress(first(payloads[index][1], 3))
        hash_field = None
        for tag, value in sequence:
            if tag == 25:
                hash_field = value
                break
        require(hash_field is not None and len(hash_field) >= 4, 'Missing file integrity record.')
        require(struct.unpack_from('<I', hash_field)[0] == 1, 'Unsupported file integrity record.')
        hash_kind, expected = next(fields(hash_field[4:]))
        require(hash_kind == 37 and hashlib.sha1(raw).digest() == expected,
                'Installed-file integrity check failed.')
        require('..' not in PurePosixPath(name).parts, 'Invalid installed path.')
        require(name not in installed or installed[name] == raw, 'Conflicting installed files.')
        installed[name] = raw
        if name.endswith('/pgr.s3e'):
            require(private is None, 'Multiple game directories found.')
            private = name.rsplit('/', 1)[0] + '/'
    require(private is not None, 'The package does not contain PGR game data.')
    game = {}
    for name, raw in installed.items():
        if name.startswith(private):
            relative = name[len(private):]
            require(relative in GAME_FILES and '/' not in relative, 'Unexpected game-data filename.')
            game[relative] = raw
    require('pgr.s3e' in game, 'Missing game program.')
    decoder = zlib.decompressobj(16 + zlib.MAX_WBITS)
    program = decoder.decompress(game['pgr.s3e'], 16 * 1024 * 1024 + 1)
    require(decoder.eof and not decoder.unconsumed_tail and not decoder.unused_data,
            'Incomplete game program stream.')
    require(program[:4] == b'XE3U', 'Unexpected game program format.')
    game['pgr.xe3u'] = program
    require(set(game) == set(GAME_FILES), 'Game-data inventory differs from this release.')
    for name, raw in game.items():
        require(hashlib.sha256(raw).hexdigest() == GAME_FILES[name], 'Game-data check failed: ' + name)
    return game


def install_data(game, destination, force=False):
    destination = Path(destination).expanduser()
    conflicts = []
    for name, expected in GAME_FILES.items():
        target = destination / name
        require(not target.is_symlink(), 'Output file is a symbolic link: ' + str(target))
        if target.exists():
            require(target.is_file(), 'Output path is not a file: ' + str(target))
            if hashlib.sha256(target.read_bytes()).hexdigest() != expected:
                conflicts.append(name)
    require(not conflicts or force,
            'Existing files differ: ' + ', '.join(conflicts) + '. Use a fresh output folder or --force to replace only these game files.')
    destination.mkdir(parents=True, exist_ok=True)
    for name, raw in game.items():
        target = destination / name
        if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest() == GAME_FILES[name]:
            continue
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=destination, prefix='.pgr_prepare_', delete=False) as file:
                temporary = Path(file.name)
                file.write(raw)
            os.replace(temporary, target)
        finally:
            if temporary is not None and temporary.exists():
                temporary.unlink()
    return destination


def main(argv=None):
    base = Path(__file__).resolve().parent
    default_output = base.parent / 'game' if base.name == 'tools' else base / '3ds' / 'pgr_mobile' / 'game'
    parser = argparse.ArgumentParser(description='Prepare verified Project Gotham Racing game data for the New 3DS port. Python 3.8+; no extra packages needed.')
    parser.add_argument('package', type=Path, help='The supported original Symbian SIS/SISX file.')
    parser.add_argument('--output', type=Path, default=default_output, help='Destination game folder (not the SD root).')
    parser.add_argument('--force', action='store_true', help='Replace differing recognized game files; unrelated files and saves are never removed.')
    args = parser.parse_args(argv)
    try:
        game = prepare_data(args.package)
        destination = install_data(game, args.output, args.force)
    except (PreparationError, OSError, ValueError, struct.error, StopIteration, zlib.error) as error:
        print('Unable to prepare game data: ' + str(error), file=sys.stderr)
        return 1
    print('Prepared and verified %d game files in %s' % (len(game), destination))
    print('Keep your existing user/ save folder. Copy the kit\'s 3ds/ folder to the SD card to finish installation.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
