import json,subprocess,sys
from pathlib import Path
OUT=Path(__file__).resolve().parents[1]
prof=Path(r'D:\Bambu Studio\resources\profiles\BBL')
cfg=OUT/'检查记录'/'切片验证';cfg.mkdir(exist_ok=True)
index={p.stem:p for p in prof.rglob('*.json')}
def resolve(name,seen=None):
    seen=set() if seen is None else set(seen)
    assert name not in seen,name
    seen.add(name);d=json.loads(index[name].read_text(encoding='utf-8'))
    full=resolve(d['inherits'],seen) if d.get('inherits') else {}
    for inc in d.get('include',[]):full.update(resolve(inc,seen))
    full.update(d);full.pop('inherits',None);full.pop('include',None)
    return full
machine=resolve('Bambu Lab A1 mini 0.4 nozzle')
process=resolve('0.20mm Standard @BBL A1M')
process.update(wall_loops='5',top_shell_layers='6',bottom_shell_layers='6',sparse_infill_density='40%',sparse_infill_pattern='gyroid',brim_type='no_brim',skirt_loops='0',curr_bed_type='Textured PEI Plate',enable_support='1',support_type='normal(auto)')
filament=resolve('Generic PETG @BBL A1M')
for name,d in [('machine',machine),('process',process),('filament',filament)]:
    (cfg/(name+'.json')).write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf-8')
manifest=json.loads((OUT/'检查记录'/'零件清单.json').read_text(encoding='utf-8'))
prior=json.loads((OUT/'检查记录'/'Bambu切片验证.json').read_text(encoding='utf-8')) if (OUT/'检查记录'/'Bambu切片验证.json').exists() else []
if '--remaining' in sys.argv:
    done={p['part'] for p in prior if p['returncode']==0}
    names=[p['name'] for p in manifest if p['printed'] and p['name'] not in done]
else:names=sys.argv[1:] or [p['name'] for p in manifest if p['printed']]
results=[p for p in prior if p['part'] not in names]
for name in names:
    work=cfg/name;work.mkdir(exist_ok=True)
    cmd=[r'D:\Bambu Studio\bambu-studio.exe','--debug','2','--load-settings',str(cfg/'machine.json')+';'+str(cfg/'process.json'),'--load-filaments',str(cfg/'filament.json'),'--arrange','0','--ensure-on-bed','--slice','0','--outputdir',str(work),str(OUT/'STL打印'/(name+'.3mf'))]
    with (cfg/(name+'.log')).open('wb') as log:
        try:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=90)
        except subprocess.TimeoutExpired:
            results.append({'part':name,'returncode':'timeout','seconds':90});print(name,'TIMEOUT',flush=True);continue
    results.append({'part':name,'returncode':r.returncode,'profile':'A1 mini 0.4 / Generic PETG / Textured PEI / 0.20mm / 5 walls / 40% gyroid','supports':'normal(auto)'})
    print(name,r.returncode,(cfg/(name+'.log')).read_text(encoding='utf-8',errors='replace')[-1500:],flush=True)
(OUT/'检查记录'/'Bambu切片验证.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')

