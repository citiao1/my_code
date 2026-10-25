"""Offline evidence only: NEVER feed reconstructed bytes to a controller."""
import argparse
import json
from pathlib import Path


def analyze(capture):
    sessions = [capture]
    sessions += [i['evidence'] for i in capture.get('incidentHistory', {}).get('incidents', [])
                 if 'evidence' in i]
    findings = []
    seen = set()
    for session in sessions:
        for row in session.get('trace', []):
            if row.get('event') != 'rx':
                continue
            raw = bytes.fromhex(row['detail'])
            if not any(b > 126 or (b < 32 and b not in (10, 13)) for b in raw):
                continue
            key = (row['hostMs'], raw)
            if key in seen:
                continue
            seen.add(key)
            findings.append({
                'hostMs': row['hostMs'], 'rawHex': raw.hex(), 'bytes': len(raw),
                'lowThreeBits010': sum((b & 7) == 2 for b in raw),
                'hypothesisAsciiLowFiveBits': ''.join(chr((b >> 3) | 0x20) for b in raw),
            })
    return {'note': 'Offline hypothesis only; upper bits are lost. Not a valid frame or proof of which device corrupted it.',
            'bytes': sum(r['bytes'] for r in findings),
            'lowThreeBits010': sum(r['lowThreeBits010'] for r in findings), 'chunks': findings}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('output exists; choose a fresh path')
    result = analyze(json.loads(args.capture.read_text(encoding='utf-8')))
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
