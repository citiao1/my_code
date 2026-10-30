exec((__import__('pathlib').Path(__file__).parent/'sw_probe.py').read_text(encoding='utf-8'))
import math,json
OUT=Path(__file__).resolve().parents[1]
doc=None
for d in sw.GetDocuments() or []:
    d=mod.IModelDoc2(d._oleobj_)
    if d.GetTitle()=='V3_Furuta_DirectDrive_297.SLDASM':doc=d
assert doc
asm=mod.IAssemblyDoc(doc._oleobj_);mu=mod.IMathUtility(sw.GetMathUtility()._oleobj_)
cs={}
for c in asm.GetComponents(True):
    c=mod.IComponent2(c._oleobj_)
    for key,prefix in [('rotor','V3_A02'),('pendulum','V3_A03')]:
        if c.Name2.startswith(prefix):cs[key]=c
I=[[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]]
def move(key,R,t):
    c=cs[key];assert not c.IsFixed()
    arr=[R[j][i] for i in range(3) for j in range(3)]+[v/1000 for v in t]+[1.,0.,0.,0.]
    mt=mu.CreateTransform(win32com.client.VARIANT(pythoncom.VT_ARRAY|pythoncom.VT_R8,arr))
    ok=c.SetTransformAndSolve2(mt)
    actual=list(mod.IMathTransform(c.Transform2._oleobj_).ArrayData)
    err=max(abs(a-b) for a,b in zip(arr[:12],actual[:12]))
    assert ok and err<1e-7,(key,ok,err,actual,arr)
    return {'component':key,'solve_success':bool(ok),'max_transform_error':err}
a=math.radians(25);b=math.radians(20)
Rz=[[math.cos(a),-math.sin(a),0.],[math.sin(a),math.cos(a),0.],[0.,0.,1.]]
Rx=[[1.,0.,0.],[0.,math.cos(b),-math.sin(b)],[0.,math.sin(b),math.cos(b)]]
R=[[sum(Rz[i][k]*Rx[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
v=[0.,154*math.sin(b),154*(1-math.cos(b))]
t=[sum(Rz[i][k]*v[k] for k in range(3)) for i in range(3)]
checks=[]
try:
    checks.append(move('rotor',Rz,[0.,0.,0.]))
    checks.append(move('pendulum',R,t))
finally:
    checks.append(move('rotor',I,[0.,0.,0.]))
    checks.append(move('pendulum',I,[0.,0.,0.]))
doc.ForceRebuild3(False);assert doc.Save3(1,0,0)[0]
r={'tests':checks,'rotor_test_deg':25,'pendulum_relative_test_deg':20,'neutral_pose_restored':True,'mechanical_stops':False,'limit_mates':False,'limits':'Rigid-body mate solve only; does not verify real cable behavior.'}
(OUT/'检查记录'/'自由转动验证.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(r,ensure_ascii=False),flush=True)
