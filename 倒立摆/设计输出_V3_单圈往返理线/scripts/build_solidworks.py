exec((__import__('pathlib').Path(__file__).parent/'sw_probe.py').read_text(encoding='utf-8'))
import json, traceback, time
OUT=Path(__file__).resolve().parents[1]; SW=OUT/'SolidWorks'
manifest=json.loads((OUT/'检查记录'/'零件清单.json').read_text(encoding='utf-8'))
instances=json.loads((OUT/'检查记录'/'装配实例.json').read_text(encoding='utf-8'))
report={'version':sw.RevisionNumber(),'parts':[],'subassemblies':[],'mates':[]}
null=win32com.client.VARIANT(pythoncom.VT_DISPATCH,None)
mathutil=mod.IMathUtility(sw.GetMathUtility()._oleobj_)
def save(doc,path):
    doc.ForceRebuild3(False)
    res=doc.Extension.SaveAs(str(path),0,1,None,0,0)
    print('SAVE',path.name,res,flush=True)
    if not res[0] or res[1]:raise RuntimeError((path,res))
    return res
def activate(doc):sw.ActivateDoc3(doc.GetTitle(),False,0,0)
def planes(doc):
    fs=[];f=doc.FirstFeature()
    while f:
        f=mod.IFeature(f._oleobj_)
        if f.GetTypeName2()=='RefPlane':fs.append(f)
        f=f.GetNextFeature()
    return fs
def addaxis(doc,name,sourcecomp=None):
    doc.ClearSelection2(True)
    if sourcecomp is None:
        ps=planes(doc)
        ps[1].Select2(False,0);ps[2].Select2(True,0)
    else:
        pd=mod.IPartDoc(sourcecomp.GetModelDoc2()._oleobj_)
        found=None
        for body in pd.GetBodies2(0,False):
            body=mod.IBody2(body._oleobj_)
            for face in body.GetFaces():
                face=mod.IFace2(face._oleobj_)
                surf=mod.ISurface(face.GetSurface()._oleobj_)
                if surf.IsCylinder():
                    cp=surf.CylinderParams
                    if abs(cp[3])>.99 and abs(cp[1])<0.0001 and abs(cp[2]-0.154)<0.0001:
                        found=sourcecomp.GetCorrespondingEntity(face);break
            if found:break
        if not found:raise RuntimeError('No radial cylindrical face')
        mod.IEntity(found._oleobj_).Select4(False,None)
    ok=doc.InsertAxis2(True)
    assert ok,('axis',name)
    f=doc.FirstFeature();last=None
    while f:
        f=mod.IFeature(f._oleobj_)
        if f.GetTypeName2()=='RefAxis':last=f
        f=f.GetNextFeature()
    assert last
    last.Name=name;doc.ClearSelection2(True)
def transform(comp,xyz,R=None):
    R=R or [[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]]
    arr=[R[j][i] for i in range(3) for j in range(3)]+[xyz[0]/1000,xyz[1]/1000,xyz[2]/1000,1.,0.,0.,0.]
    comp.Transform2=mathutil.CreateTransform(win32com.client.VARIANT(pythoncom.VT_ARRAY|pythoncom.VT_R8,arr))
docs={}
known={i['name'].lower() for i in manifest}|{'p10_fork_inner','p10_fork_outer'}
open_titles=[]
for opened in ([] if '--assembly-only' in sys.argv else sw.GetDocuments() or []):
    opened=mod.IModelDoc2(opened._oleobj_)
    if opened.GetTitle().startswith('V3_') and str(OUT) in opened.GetPathName():open_titles.append(opened.GetTitle())
for title in sorted(open_titles,key=lambda t:0 if t.endswith('.SLDASM') else 1):
    print('CLOSE',title,flush=True);sw.CloseDoc(title)
for item in manifest:
    name=item['name'];path=SW/(name+'.SLDPRT')
    if name=='H07_Collar8_D12H4':continue
    if '--assembly-only' in sys.argv:
        found=None
        for opened in sw.GetDocuments() or []:
            opened=mod.IModelDoc2(opened._oleobj_)
            if opened.GetTitle().lower()==path.name.lower():found=opened;break
        if found is None:found=sw.OpenDoc6(str(path),1,1,'',0,0)[0]
        assert found is not None,name
        docs[name]=found
        report['parts'].append({'name':name,'saved':True,'solid_bodies':len(mod.IPartDoc(found._oleobj_).GetBodies2(0,False))})
        continue
    existing=sw.GetOpenDocumentByName(str(path))
    if existing:sw.CloseDoc(mod.IModelDoc2(existing._oleobj_).GetTitle())
    # Reimport to ensure revised parts are up to date.
    doc,err=sw.LoadFile4(str(OUT/'STEP零件'/(name+'.step')),'',None,0)
    assert doc is not None,(name,err)
    bodies=mod.IPartDoc(doc._oleobj_).GetBodies2(0,False)
    assert len(bodies)>=1,(name,len(bodies))
    if item['printed']:assert len(bodies)==1,(name,len(bodies))
    col=(.12,.40,.64) if item['printed'] else (.66,.68,.72)
    if item['group']=='pendulum':col=(.96,.48,.08)
    doc.MaterialPropertyValues=win32com.client.VARIANT(pythoncom.VT_ARRAY|pythoncom.VT_R8,[*col,.3,.6,.4,.25,0.,0.])
    doc.Extension.CustomPropertyManager('').Add3('Source',30,'PETG / A1 mini' if item['printed'] else 'Purchased / original STEP',1)
    doc.Extension.CustomPropertyManager('').Add3('Notes',30,item['note'],1)
    doc.ShowNamedView2('*Isometric',7);doc.ViewZoomtofit2()
    save(doc,path);docs[name]=doc
    report['parts'].append({'name':name,'solid_bodies':len(bodies),'import_error':err,'saved':True})
template=r'C:\ProgramData\SOLIDWORKS\SOLIDWORKS 2024\templates\gb_assembly.asmdot'
subs={}
for group,filename in [('stationary','V3_A01_Stationary'),('rotor','V3_A02_Rotor'),('pendulum','V3_A03_Pendulum')]:
    doc=mod.IModelDoc2(sw.NewDocument(template,0,0.,0.)._oleobj_);asm=mod.IAssemblyDoc(doc._oleobj_)
    comps={}
    for inst in instances:
        if inst['group']!=group:continue
        name=inst['name'];activate(doc)
        comp=asm.AddComponent5(str(SW/(name+'.SLDPRT')),0,'',False,'',0.,0.,0.)
        assert comp,(group,name)
        comp.Select4(False,None,False);asm.UnfixComponent()
        transform(comp,inst['translation'],inst.get('rotation'));comp.Select4(False,None,False);asm.FixComponent()
        comps[name]=comp
    addaxis(doc,'JointAxis_Z')
    if group=='rotor':addaxis(doc,'JointAxis_Pendulum',comps['V3_E02_WDD35D4'])
    if group=='pendulum':addaxis(doc,'JointAxis_Pendulum',comps['V3_P07_LightPendulum_135'])
    doc.ShowNamedView2('*Isometric',7);doc.ViewZoomtofit2();save(doc,SW/(filename+'.SLDASM'))
    subs[group]=(filename,doc)
    report['subassemblies'].append({'name':filename,'components':asm.GetComponentCount(False)})
doc=mod.IModelDoc2(sw.NewDocument(template,0,0.,0.)._oleobj_);asm=mod.IAssemblyDoc(doc._oleobj_)
topcomps={}
for group in ['stationary','rotor','pendulum']:
    name,sub=subs[group];activate(doc)
    c=asm.AddComponent5(str(SW/(name+'.SLDASM')),0,'',False,'',0.,0.,0.)
    assert c
    c.Select4(False,None,False);asm.UnfixComponent();transform(c,[0,0,0])
    if group=='stationary':c.Select4(False,None,False);asm.FixComponent()
    topcomps[group]=c
doc.ClearSelection2(True)
topplanes=planes(doc)
topname='V3_Furuta_DirectDrive_297'
save(doc,SW/(topname+'.SLDASM'))
def selectfeature(group,name,kind,append):
    c=topcomps[group]
    # Selection names for reference geometry in assembly components.
    path=f'{name}@{c.Name2}@{doc.GetTitle().rsplit(".",1)[0]}'
    ok=doc.Extension.SelectByID2(path,kind,0.,0.,0.,append,0,None,0)
    if not ok:
        path=f'{name}@{c.Name2}'
        ok=doc.Extension.SelectByID2(path,kind,0.,0.,0.,append,0,None,0)
    if not ok:raise RuntimeError(('selection',path))
def mate(g1,n1,g2,n2,kind,mt,title):
    doc.ClearSelection2(True);selectfeature(g1,n1,kind,False);selectfeature(g2,n2,kind,True)
    res=asm.AddMate5(mt,2,False,0.,0.,0.,0.,0.,0.,0.,0.,False,False,0)
    obj,err=res if isinstance(res,tuple) else (res,-999)
    assert obj is not None and err==1,(title,res)
    report['mates'].append({'name':title,'error':err})
    doc.ClearSelection2(True)
try:
    mate('stationary','JointAxis_Z','rotor','JointAxis_Z','AXIS',0,'Main revolute axis')
    mate('stationary',planes(subs['stationary'][1])[0].Name,'rotor',planes(subs['rotor'][1])[0].Name,'PLANE',0,'Rotor axial position')
    mate('rotor','JointAxis_Pendulum','pendulum','JointAxis_Pendulum','AXIS',0,'Pendulum revolute axis')
    mate('rotor',planes(subs['rotor'][1])[2].Name,'pendulum',planes(subs['pendulum'][1])[2].Name,'PLANE',0,'Pendulum axial position')
except Exception as e:
    report['mate_error']=str(e);traceback.print_exc()
    raise
doc.ForceRebuild3(False);doc.ShowNamedView2('*Isometric',7);doc.ViewZoomtofit2()
save(doc,SW/(topname+'.SLDASM'))
report['top_components']=asm.GetComponentCount(False)
report['resolved']=asm.ResolveAllLightWeightComponents(False)
try:report['preview_saved']=doc.SaveBMP(str(OUT/'预览'/'SolidWorks实际装配.bmp'),1600,1200)
except Exception as e:report['preview_error']=str(e)
(OUT/'检查记录'/'SolidWorks验证.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('DONE',json.dumps(report,ensure_ascii=False),flush=True)
titles=[]
for dd in sw.GetDocuments() or []:
    dd=mod.IModelDoc2(dd._oleobj_)
    if dd.GetTitle().startswith('V3_') and dd.GetTitle()!=topname+'.SLDASM':titles.append(dd.GetTitle())
for title in titles:sw.CloseDoc(title)
activate(doc)

