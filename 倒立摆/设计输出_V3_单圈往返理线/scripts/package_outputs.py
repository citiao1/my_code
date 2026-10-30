import json,zipfile,hashlib
from pathlib import Path
OUT=Path(__file__).resolve().parents[1];ROOT=OUT.parent
m=json.loads((OUT/'检查记录'/'零件清单.json').read_text(encoding='utf-8'))
s=json.loads((OUT/'检查记录'/'Bambu切片验证.json').read_text(encoding='utf-8'))
assert len(s)==13 and all(x['returncode']==0 for x in s)
for x in s:
    d=OUT/'检查记录'/'切片验证'/x['part']
    result=json.loads((d/'result.json').read_text(encoding='utf-8'))
    assert result['return_code']==0 and result['sliced_plates']
    # G-code is disposable validation output; archive the slicer's result instead.
    if (d/'plate_1.gcode').exists():assert (d/'plate_1.gcode').stat().st_size>5000
assert json.loads((OUT/'检查记录'/'干涉检查.json').read_text(encoding='utf-8'))==[]
motion=json.loads((OUT/'检查记录'/'运动检查.json').read_text(encoding='utf-8'))
assert motion['pendulum_collisions']==[] and motion['rotary_collisions']==[]
v=json.loads((OUT/'检查记录'/'SolidWorks最终验证.json').read_text(encoding='utf-8'))
free=json.loads((OUT/'检查记录'/'自由转动验证.json').read_text(encoding='utf-8'))
assert all(x['solve_success'] and x['max_transform_error']<1e-7 for x in free['tests'])
assert len(v['mates'])==4 and all(x['feature_error'][0]==0 for x in v['mates'])
assert not v['mechanical_hard_stops'] and not v['limit_angle_mates']
files=[]
for row in m:
    n=row['name'];files +=[OUT/'STEP零件'/(n+'.step'),OUT/'SolidWorks'/(n+'.SLDPRT')]
    if row['printed']:files +=[OUT/'STL打印'/(n+'.stl'),OUT/'STL打印'/(n+'.3mf')]
files +=[OUT/'SolidWorks'/(n+'.SLDASM') for n in ['V3_A01_Stationary','V3_A02_Rotor','V3_A03_Pendulum','V3_Furuta_DirectDrive_297']]
files +=[OUT/'V3_倒立摆_完整装配.step',OUT/'打印与装配说明_V3.md']
files +=list((OUT/'预览').glob('*.png'))+list((OUT/'预览').glob('*.svg'))
if (OUT/'预览'/'SolidWorks实际装配.bmp').exists():files.append(OUT/'预览'/'SolidWorks实际装配.bmp')
for n in ['零件清单.json','装配实例.json','干涉检查.json','运动检查.json','Bambu切片验证.json','SolidWorks最终验证.json','导线名义运动.json','自由转动验证.json']:
    files.append(OUT/'检查记录'/n)
for x in s:files.append(OUT/'检查记录'/'切片验证'/x['part']/'result.json')
files +=list((OUT/'scripts').glob('*.py'))
assert all(p.exists() for p in files)
rows=[{'file':p.relative_to(OUT).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in files]
p=OUT/'检查记录'/'交付文件校验.json';p.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8');files.append(p)
path=ROOT/'倒立摆_V3_单圈往返理线_A1mini_PETG.zip'
with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
    for p in files:z.write(p,arcname='倒立摆_V3/'+p.relative_to(OUT).as_posix())
with zipfile.ZipFile(path) as z:assert z.testzip() is None
print(path,len(files),path.stat().st_size)
