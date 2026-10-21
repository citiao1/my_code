"""Validate and score one original R20 WLOG raw JSON without connecting hardware."""
import argparse, json
from dataclasses import asdict
from pathlib import Path
from wheel_log import VERSION, CAPS, INFO, score

def replay(path):
    raw=json.loads(path.read_text(encoding='utf-8'))
    if raw.get('firmware')!=VERSION or raw.get('caps')!=CAPS or raw.get('controller')!=INFO or raw.get('source')!='MCU_WLOG':
        raise ValueError('raw WLOG provenance/profile mismatch')
    if raw.get('quality')=='failed' or raw.get('error'):
        raise ValueError('failed original trial is diagnostic only')
    _,result=score(raw['records'],raw['wheel'],raw['target'],raw['baseline_seconds'],raw['seconds'])
    return {'source':str(path),'evidence':'historical_MCU_WLOG_replay','metrics':asdict(result),
            'candidate_proposed':False,'hardware_connected':False,'gains_written':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('raw',type=Path)
    args=p.parse_args();print(json.dumps(replay(args.raw),indent=2))
