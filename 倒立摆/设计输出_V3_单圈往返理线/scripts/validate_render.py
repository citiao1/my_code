import sys,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'.cad_runtime'))
import cadquery as cq
import vtk
OUT=ROOT/'设计输出_V3_单圈往返理线'
manifest={i['name']:i for i in json.loads((OUT/'检查记录'/'零件清单.json').read_text(encoding='utf-8'))}
items=json.loads((OUT/'检查记录'/'装配实例.json').read_text(encoding='utf-8'))
shapes={n:cq.importers.importStep(str(OUT/'STEP零件'/(n+'.step'))).val() for n in manifest}
inst=[(i,shapes[i['name']].moved(cq.Location(cq.Plane(origin=tuple(i['translation']),xDir=tuple(row[0] for row in i['rotation']),normal=tuple(row[2] for row in i['rotation']))))) for i in items]
def overlapping(a,b):
    p=a.BoundingBox();q=b.BoundingBox()
    return not any([p.xmax<=q.xmin+.001,q.xmax<=p.xmin+.001,p.ymax<=q.ymin+.001,q.ymax<=p.ymin+.001,p.zmax<=q.zmin+.001,q.zmax<=p.zmin+.001])
rod=shapes['V3_P07_LightPendulum_135'];collisions=[]
for angle in range(0,360,5):
    moved=rod.rotate((156,0,154),(157,0,154),angle)
    for i,s in inst:
        if i['group']=='pendulum':continue
        if overlapping(moved,s):
            v=moved.intersect(s).Volume()
            if v>.02:collisions.append([angle,i['name'],round(v,5)])
result={'static_interferences':json.loads((OUT/'检查记录'/'干涉检查.json').read_text(encoding='utf-8')),'pendulum_sweep_degrees':list(range(0,360,5)),'pendulum_collisions':collisions,'limits':'Exact BRep intersections at 5-degree samples; rods/fasteners not all modeled; no load/dynamic validation.'}
rotary=[]
for angle in range(-180,181,15):
    for i,s in inst:
        if i['group'] not in ['rotor','pendulum']:continue
        moved=s.rotate((0,0,0),(0,0,1),angle)
        for j,t in inst:
            if j['group']!='stationary' or not overlapping(moved,t):continue
            v=moved.intersect(t).Volume()
            if v>.025:rotary.append([angle,i['label'],j['label'],round(v,5)])
result.update(rotary_sweep_degrees=list(range(-180,181,15)),rotary_collisions=rotary)
result['limits']='Sampled exact BRep intersections: pendulum 5 degrees, rotary assembly 15 degrees. Original motor/sensor fastening screws omitted. No load, cable force, fatigue, real contact, or physical testing.'
print(json.dumps(result),flush=True)
(OUT/'检查记录'/'运动检查.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
def actor(s,color):
    vs,ts=s.tessellate(.12,.2);pts=vtk.vtkPoints();cells=vtk.vtkCellArray()
    for p in vs:pts.InsertNextPoint(p.x,p.y,p.z)
    for t in ts:
        tri=vtk.vtkTriangle()
        for k in range(3):tri.GetPointIds().SetId(k,t[k])
        cells.InsertNextCell(tri)
    poly=vtk.vtkPolyData();poly.SetPoints(pts);poly.SetPolys(cells)
    norms=vtk.vtkPolyDataNormals();norms.SetInputData(poly);norms.SetFeatureAngle(45)
    mapper=vtk.vtkPolyDataMapper();mapper.SetInputConnection(norms.GetOutputPort())
    a=vtk.vtkActor();a.SetMapper(mapper);a.GetProperty().SetColor(*color);a.GetProperty().SetSpecular(.3);a.GetProperty().SetSpecularPower(35)
    return a
def render(filename,down=False,exploded=False,view='iso'):
    renderer=vtk.vtkRenderer();renderer.SetBackground(.96,.97,.985)
    for i,s in inst:
        g=i['group'];name=i['name'];p=manifest[name]
        color=(.10,.39,.62) if p['printed'] else (.58,.61,.65)
        if g=='pendulum':color=(.98,.48,.08)
        if name=='E01_MG513_Original':color=(.32,.34,.37)
        if name=='E02_WDD35D4_Original':color=(.6,.63,.66)
        if down and g=='pendulum':s=s.rotate((156,0,154),(157,0,154),180)
        if exploded:
            if name.startswith('V3_P01'):offset=(0,0,-50)
            elif name.startswith(('V3_P02','V3_E01','V3_P03')):offset=(0,0,-20)
            elif g=='rotor':offset=(0,0,80)
            elif g=='pendulum':offset=(65,0,80)
            else:offset=(0,0,20)
            s=s.translate(offset)
        renderer.AddActor(actor(s,color))
    cam=renderer.GetActiveCamera();cam.SetViewUp(0,0,1);cam.ParallelProjectionOn()
    if view=='side':cam.SetPosition(600,500,300);cam.SetFocalPoint(60,0,130)
    else:cam.SetPosition(530,-650,430);cam.SetFocalPoint(60,0,145)
    renderer.ResetCamera();cam.Zoom(1.12)
    win=vtk.vtkRenderWindow();win.SetOffScreenRendering(1);win.SetSize(1500,1500);win.AddRenderer(renderer);win.SetMultiSamples(4);win.Render()
    capture=vtk.vtkWindowToImageFilter();capture.SetInput(win);capture.Update()
    writer=vtk.vtkPNGWriter();writer.SetFileName(str(OUT/'预览'/filename));writer.SetInputConnection(capture.GetOutputPort());writer.Write();win.Finalize()
render('装配预览.png');render('摆杆下垂.png',down=True);render('分层装配示意.png',exploded=True)

from wire_path import pose,Z_LEVELS
def wireactor(points,color):
    pts=vtk.vtkPoints();line=vtk.vtkPolyLine();line.GetPointIds().SetNumberOfIds(len(points))
    for k,p in enumerate(points):pts.InsertNextPoint(*p);line.GetPointIds().SetId(k,k)
    cells=vtk.vtkCellArray();cells.InsertNextCell(line)
    pd=vtk.vtkPolyData();pd.SetPoints(pts);pd.SetLines(cells)
    tube=vtk.vtkTubeFilter();tube.SetInputData(pd);tube.SetRadius(.5);tube.SetNumberOfSides(12)
    mp=vtk.vtkPolyDataMapper();mp.SetInputConnection(tube.GetOutputPort())
    a=vtk.vtkActor();a.SetMapper(mp);a.GetProperty().SetColor(*color);return a
def cable_render(angle):
    ren=vtk.vtkRenderer();ren.SetBackground(.965,.975,.99)
    for i,s in inst:
        if i['name'] not in ['V3_P09_FixedCableTray','V3_P10_RotatingCableCover','V3_P11_MovingWireKeeper','V3_P12_FixedWireClamp']:continue
        if i['group']=='rotor':s=s.rotate((0,0,0),(0,0,1),angle)
        a=actor(s,(.16,.44,.65) if i['group']=='stationary' else (.48,.69,.8))
        if i['name']=='V3_P10_RotatingCableCover':a.GetProperty().SetOpacity(.12)
        ren.AddActor(a)
    for z,col in zip(Z_LEVELS,[(.8,.08,.1),(.05,.35,.85),(.1,.12,.15)]):ren.AddActor(wireactor(pose(angle,z)['points'],col))
    cam=ren.GetActiveCamera();cam.SetViewUp(0,1,0);cam.SetPosition(50,-90,400);cam.SetFocalPoint(0,0,110);cam.ParallelProjectionOn();ren.ResetCamera();cam.Zoom(1.1)
    win=vtk.vtkRenderWindow();win.SetOffScreenRendering(1);win.SetSize(1400,1200);win.AddRenderer(ren);win.SetMultiSamples(4);win.Render()
    capture=vtk.vtkWindowToImageFilter();capture.SetInput(win);capture.Update()
    writer=vtk.vtkPNGWriter();writer.SetFileName(str(OUT/'预览'/f'理线机构_{angle:+d}度.png'));writer.SetInputConnection(capture.GetOutputPort());writer.Write();win.Finalize()
for a in [-180,0,180]:cable_render(a)

