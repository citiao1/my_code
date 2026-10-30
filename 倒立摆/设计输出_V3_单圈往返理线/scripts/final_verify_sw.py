exec((__import__('pathlib').Path(__file__).parent/'sw_probe.py').read_text(encoding='utf-8'))
import json,math
OUT=Path(__file__).resolve().parents[1];SW=OUT/'SolidWorks'
report={'version':sw.RevisionNumber(),'components':[],'mates':[]}
constants=win32com.client.gencache.MakeModuleForTypelibInterface(pythoncom.LoadTypeLib(r'D:\solidworks\SOLIDWORKS\swconst.tlb'))
report['add_mate_success_enum']=getattr(constants.constants,'swAddMateError_None',None)
doc=None
for dd in sw.GetDocuments() or []:
    dd=mod.IModelDoc2(dd._oleobj_)
    if dd.GetTitle().lower()=='v3_furuta_directdrive_297.sldasm':doc=dd
assert doc is not None
sw.ActivateDoc3(doc.GetTitle(),False,0,0)
asm=mod.IAssemblyDoc(doc._oleobj_)
mu=mod.IMathUtility(sw.GetMathUtility()._oleobj_)
identity=mu.CreateTransform(win32com.client.VARIANT(pythoncom.VT_ARRAY|pythoncom.VT_R8,[1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.]))
for prefix in ['V3_A02_Rotor','V3_A03_Pendulum']:
    for c in asm.GetComponents(True):
        c=mod.IComponent2(c._oleobj_)
        if c.Name2.startswith(prefix):
            report[prefix+'_pose_reset']=c.SetTransformAndSolve2(identity)
report['rebuild_result']=doc.ForceRebuild3(False)
for c in asm.GetComponents(False):
    c=mod.IComponent2(c._oleobj_)
    t=mod.IMathTransform(c.Transform2._oleobj_).ArrayData
    path=c.GetPathName().replace('\\\\','\\')
    row={'name':c.Name2,'path':path,'exists':Path(path).exists(),'suppression':c.GetSuppression2(),'transform':list(t),'fixed':c.IsFixed()}
    assert row['exists'],row
    if c.Name2.startswith(('V3_A02','V3_A03')):assert not c.IsFixed(),row
    report['components'].append(row)
f=doc.FirstFeature()
while f:
    f=mod.IFeature(f._oleobj_)
    if f.GetTypeName2()=='MateGroup':
        sf=f.GetFirstSubFeature()
        while sf:
            sf=mod.IFeature(sf._oleobj_)
            obj=sf.GetSpecificFeature2()
            if obj:
                try:
                    m=mod.IMate2(obj._oleobj_)
                    row={'name':sf.Name,'type':m.Type,'entities':m.GetMateEntityCount(),'feature_error':sf.GetErrorCode2()}
                    report['mates'].append(row)
                except Exception:pass
            sf=sf.GetNextSubFeature()
    f=f.GetNextFeature()
# A proper Z-up saved orientation for this machine, whose CAD coordinates use Z as vertical.
eye=[.50,-.74,.45];ln=math.sqrt(sum(v*v for v in eye));n=[v/ln for v in eye]
r=[-n[1],n[0],0.];ln=math.sqrt(sum(v*v for v in r));r=[v/ln for v in r]
u=[n[1]*r[2]-n[2]*r[1],n[2]*r[0]-n[0]*r[2],n[0]*r[1]-n[1]*r[0]]
arr=[r[0],u[0],n[0],r[1],u[1],n[1],r[2],u[2],n[2],0.,0.,0.,1.,0.,0.,0.]
mu=mod.IMathUtility(sw.GetMathUtility()._oleobj_)
view=mod.IModelView(doc.ActiveView._oleobj_)
view.Orientation3=mu.CreateTransform(win32com.client.VARIANT(pythoncom.VT_ARRAY|pythoncom.VT_R8,arr))
doc.ViewZoomtofit2();doc.GraphicsRedraw2()
try:doc.NameView('V3_Z_UP_Assembly')
except Exception:pass
for dd in sw.GetDocuments() or []:
    dd=mod.IModelDoc2(dd._oleobj_)
    if dd.GetType()==2 and str(OUT) in dd.GetPathName():
        rr=dd.Save3(1,0,0);assert rr[0],(dd.GetTitle(),rr)
report['top_save']=doc.Save3(1,0,0)
report['dependencies']=doc.GetDependencies2(True,True,False)
assert len(report['mates'])==4,report['mates']
assert all(m['entities']==2 and m['feature_error'][0]==0 for m in report['mates']),report['mates']
assert all(m['type']==0 for m in report['mates']),report['mates']
report['mechanical_hard_stops']=False
report['limit_angle_mates']=False
doc.SaveBMP(str(OUT/'预览'/'SolidWorks实际装配.bmp'),1400,1100)
(OUT/'检查记录'/'SolidWorks最终验证.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False),flush=True)

