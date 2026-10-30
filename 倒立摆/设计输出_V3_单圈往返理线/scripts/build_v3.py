import sys,json,math,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'.cad_runtime'))
import cadquery as cq
import trimesh
import numpy as np
OUT=Path(__file__).resolve().parents[1]
parts={};instances=[]
def box(x,y,z,dx,dy,dz):return cq.Workplane('XY').box(dx,dy,dz,centered=False).translate((x,y,z)).val()
def cyl(r,h,p=(0,0,0),axis=(0,0,1)):return cq.Solid.makeCylinder(r,h,cq.Vector(*p),cq.Vector(*axis))
def hexagon(af,h,z=0):return cq.Workplane('XY').polygon(6,af/math.cos(math.pi/6)).extrude(h).translate((0,0,z)).val()
def drill(s,points,r,z,h):
    for x,y in points:s=s.cut(cyl(r,h,(x,y,z)))
    return s
def part(name,s,group='stationary',printed=True,orient=None,qty=1,note='',add=True):
    s=s.clean();assert s.isValid(),name
    assert len(s.Solids())==1,(name,len(s.Solids()))
    cq.exporters.export(s,str(OUT/'STEP零件'/(name+'.step')))
    row={'name':name,'group':group,'printed':printed,'quantity':qty,'note':note,'volume_mm3':s.Volume()}
    if printed:
        p=orient(s) if orient else s;b=p.BoundingBox();p=p.translate((-b.xmin,-b.ymin,-b.zmin));b=p.BoundingBox()
        dims=[b.xlen,b.ylen,b.zlen];assert max(dims)<177,(name,dims)
        path=OUT/'STL打印'/(name+'.stl');cq.exporters.export(p,str(path),tolerance=.025,angularTolerance=.12)
        m=trimesh.load_mesh(str(path));assert m.is_watertight and m.is_winding_consistent,name
        row.update(print_bbox_mm=dims,mesh_watertight=True,triangles=len(m.faces))
        vs=''.join(f'<vertex x="{v[0]:.5f}" y="{v[1]:.5f}" z="{v[2]:.5f}"/>' for v in m.vertices)
        fs=''.join(f'<triangle v1="{f[0]}" v2="{f[1]}" v3="{f[2]}"/>' for f in m.faces)
        data=f'<?xml version="1.0" encoding="UTF-8"?><model unit="millimeter" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02"><resources><object id="1" name="{name}" type="model"><mesh><vertices>{vs}</vertices><triangles>{fs}</triangles></mesh></object></resources><build><item objectid="1" transform="1 0 0 0 1 0 0 0 1 {90-dims[0]/2:.5f} {90-dims[1]/2:.5f} 0"/></build></model>'
        with zipfile.ZipFile(OUT/'STL打印'/(name+'.3mf'),'w',zipfile.ZIP_DEFLATED) as f:
            f.writestr('[Content_Types].xml','<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/></Types>')
            f.writestr('_rels/.rels','<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>')
            f.writestr('3D/3dmodel.model',data)
    parts[name]=(row,s)
    if add:instance(name,group)
    print(name,flush=True)
    return s
def rotation(axis):
    z=np.array(axis,dtype=float);z/=np.linalg.norm(z)
    ref=np.array([1.,0.,0.]) if abs(z[0])<.9 else np.array([0.,1.,0.])
    x=ref-z*np.dot(ref,z);x/=np.linalg.norm(x);y=np.cross(z,x)
    return np.column_stack([x,y,z])
def instance(name,group,t=(0,0,0),R=None,label=None):
    R=np.eye(3) if R is None else np.array(R)
    loc=cq.Location(cq.Plane(origin=cq.Vector(*t),xDir=cq.Vector(*R[:,0]),normal=cq.Vector(*R[:,2])))
    s=parts[name][1].moved(loc)
    instances.append(({'name':name,'label':label or f'{name}_{len(instances)+1}','group':group,'translation':list(t),'rotation':R.tolist()},s))
def bolt(origin,axis,group,length=10):
    instance('V3_H01_M3x10',group,origin,rotation(axis))
def nut(origin,axis,group):instance('V3_H02_M3_Nut',group,origin,rotation(axis))

# Base is raised 6mm for short through-bolts; 60x56 opening leaves the plug unobstructed.
base=box(-80,-75,6,160,150,3).cut(box(-22.7,-28,5,60,56,5))
for x in [-66,66]:
    for y in [-61,61]:base=base.fuse(box(x-8,y-8,0,16,16,6))
feet=[(x,y) for x in [-42,42] for y in [-34,34]]
base=drill(base,feet,1.7,5,5)
base=drill(base,[(x,y) for x in [-66,66] for y in [-61,61]],2.2,-1,11)
part('V3_P01_OpenBase',base,orient=lambda s:s.rotate((0,0,0),(1,0,0),180),note='160x150；电机下方60x56贯通窗口；底部留6mm螺母空间')
for sign,label in [(1,'Right'),(-1,'Left')]:
    panel=box(47,-45,9,3,90,85).fuse(box(35,-45,9,15,90,3)).fuse(box(35,-45,91,15,90,3))
    panel=drill(panel,[(42,-34),(42,34)],1.7,8,5)
    panel=drill(panel,[(42,-34),(42,34)],2.25,90,5)
    if sign<0:panel=panel.mirror('YZ')
    part('V3_P02_Side_'+label,panel,orient=lambda s:s.rotate((0,0,0),(0,1,0),90 if sign>0 else -90),note='侧壁距主轴47mm，远离电机尾部插头；3mm连接耳')
mount=box(-55,-45,94,110,90,3)
mount=drill(mount,feet,2.25,93,5).cut(cyl(6.4,5,(0,0,93)))
motorholes=[(23.05,0),(-.575,13.6399),(-.575,-13.6399)]
mount=drill(mount,motorholes,1.7,93,5)
for x,y in motorholes:mount=mount.cut(cyl(3.2,1.5,(x,y,95.6)))
# Underside edge ribs stiffen the thin mounting plate without blocking the motor or connector.
for y in [-43,40]:mount=mount.fuse(box(-35,y,89,70,3,5))
# Three angle-sensor wires pass through this fixed deck, away from the motor and all bolt/rib zones.
wire_slot=cq.Workplane('XY').slot2D(18,8,0).extrude(7).translate((-22,32,92)).val()
mount=mount.cut(wire_slot)
part('V3_P03_MotorDeck',mount,orient=lambda s:s.rotate((0,0,0),(1,0,0),180),note='保留电机原孔；穿线孔18x8，中心-22/32；四角改4.5mm通孔，用M4x14同时固定线槽')

# Photo adapter envelope: generous cup clearance + two adjustable side screws, no tight estimated hex socket.
metal=cyl(6,14,(0,0,104)).fuse(hexagon(12,6,118).rotate((0,0,0),(0,0,1),30))
metal=metal.cut(cyl(3.1,14,(0,0,104))).cut(cyl(2,6,(0,0,118)))
metal=metal.cut(cyl(2,8,(0,0,110),(1,0,0)))
part('V3_E03_Existing_M4_Adapter',metal,'rotor',False,note='照片估读包络：本体约20mm、圆柱约12mm；端面置于电机面上方30mm；不是精密测量')
rootbolts=[(17.5*math.cos(a),17.5*math.sin(a)) for a in [math.pi/2,7*math.pi/6,11*math.pi/6]]
cup=cyl(21,3,(0,0,124)).fuse(cyl(13,6,(0,0,118)).cut(cyl(8.2,6,(0,0,118))))
cup=cup.cut(cyl(2.25,5,(0,0,123)));cup=drill(cup,rootbolts,1.7,123,5)
for sign in [-1,1]:
    cup=cup.cut(cyl(1.7,15,(0,0,121),(sign,0,0)))
    cup=cup.cut(box(9 if sign>0 else -11.7,-3.2,117.9,2.7,6.4,6.3))
part('V3_P04_AdjustableDriveCup',cup,'rotor',orient=lambda s:s.rotate((0,0,0),(1,0,0),180),note='端面短M4直锁，不用垫套；两颗M3x10从两侧顶紧六角接头')
arm=box(-18,-18,127,165,36,3).fuse(cyl(21,3,(0,0,127)))
arm=arm.cut(cyl(9,5,(0,0,126)));arm=drill(arm,rootbolts,1.7,126,5)
for y in [-16.5,13.5]:arm=arm.fuse(box(21,y,130,95,3,10))
sensorfeet=[(x,y) for x in [122,134] for y in [-15,15]]
arm=drill(arm,sensorfeet,1.7,126,5)
arm=drill(arm,[(32,-9),(32,9)],1.7,126,5)
for x in [60,105]:
    eye=cyl(4.5,3,(x,21.5,134.5),(1,0,0)).cut(cyl(2.5,3,(x,21.5,134.5),(1,0,0)))
    arm=arm.fuse(box(x,16,127,3,7,4.5)).fuse(eye)
part('V3_P05_RibbedArm_156',arm,'rotor',orient=lambda s:s.rotate((0,0,0),(0,0,1),45),note='156mm臂；增加两处5mm导线眼和旋转线槽盖固定孔')
# Original sensor face at X137, Z154. Its own shaft reaches X156 and carries the light pendulum directly.
seat=box(118,-20,130,22,40,3).fuse(box(137,-20,130,3,40,48))
seat=seat.cut(cyl(10.25,5,(136,0,154),(1,0,0)))
sensorholes=[(15,0),(-15,0),(7.5,12.9904),(7.5,-12.9904)]
for u,v in sensorholes:seat=seat.cut(cyl(1.4,5,(136,u,154+v),(1,0,0)))
seat=drill(seat,sensorfeet,1.7,129,5)
part('V3_P06_SensorLBracket',seat,'rotor',orient=lambda s:s.rotate((0,0,0),(0,1,0),90),note='原传感器四孔与2.8mm通孔保留，使用传感器原配螺丝')
rod=box(153,-8,154,3,16,135).fuse(cyl(8,3,(153,0,289),(1,0,0))).fuse(cyl(10,11,(145,0,154),(1,0,0)))
rod=rod.cut(cyl(3.1,13,(144,0,154),(1,0,0)))
rod=rod.cut(box(144,-.4,154,13,.8,12))
rod=rod.cut(cyl(1.7,24,(150.5,-12,161),(0,1,0)))
rod=rod.cut(cyl(3.1,9,(150.5,-12,161),(0,1,0)))
rod=rod.cut(cyl(3.3,9,(150.5,3,161),(0,1,0)))
for z in [199,244,279,289]:rod=rod.cut(cyl(1.7,5,(152,0,z),(1,0,0)))
part('V3_P07_LightPendulum_135',rod,'pendulum',orient=lambda s:s.rotate((0,0,0),(0,1,0),90),note='3mm轻摆杆；6.2mm开口夹孔直接夹传感器轴；M3x10夹紧，不加轴承或联轴器')
coupon=box(0,0,0,76,27,3)
for x,d in [(10,6.1),(26,6.2),(42,6.3),(57,3.4),(68,4.5)]:coupon=coupon.cut(cyl(d/2,5,(x,13.5,-1)))
part('V3_P08_FitCoupon',coupon,'coupon',add=False,note='6.1/6.2/6.3轴孔与3.4/4.5螺丝孔；先打印确认')
motor=cq.importers.importStep(str(next((ROOT/'3D').rglob('*MG513*.stp')))).val().rotate((0,0,0),(1,0,0),90).translate((7.3,0,94))
part('V3_E01_MG513',motor,printed=False,note='原始STEP，不改变电机安装孔；尾端距桌面40.9mm')
# local X->global Y; local Y->global Z; local Z->global -X.
R=np.array([[0,0,-1],[1,0,0],[0,-1,0]],float)
sensor=cq.importers.importStep(str(next((ROOT/'3D').rglob('*WDD35D4*.stp')))).val().moved(cq.Location(cq.Plane(origin=(137,0,154),xDir=(0,1,0),normal=(-1,0,0))))
part('V3_E02_WDD35D4',sensor,'rotor',False,note='原始STEP，直接利用原轴支撑轻摆杆')

# A rotary U-loop tray: nominal wire paths use inner R33 and outer R62.25.
# It is a finite-travel cable return, not a continuous-rotation electrical connection.
tray=cyl(68,2.4,(0,0,97)).cut(cyl(28,2.4,(0,0,97)))
tray=tray.fuse(cyl(68,22.8,(0,0,99.4)).cut(cyl(66,22.8,(0,0,99.4))))
tray=tray.fuse(cyl(30,22.8,(0,0,99.4)).cut(cyl(28,22.8,(0,0,99.4))))
tray=tray.cut(wire_slot);tray=drill(tray,feet,2.25,96,5)
# Fixed outer cable anchor, with three separate semicircular insulation grooves.
tray=tray.fuse(box(-6,62,103,12,6,17))
for zz in [107.5,109,110.5]:tray=tray.cut(cyl(.75,14,(-7,62,zz),(1,0,0)))
tray=tray.cut(cyl(1.7,12,(0,60,114.5),(0,1,0))).cut(box(-3.2,65,111.4,6.4,2.7,11.7))
tray=tray.fuse(box(-3,66,107,6,10,2))
part('V3_P09_FixedCableTray',tray,note='固定环形余线槽，外径136，内孔56；底部穿线；外端三线夹座')
cover=cyl(71,2.4,(0,0,123.2)).cut(cyl(27.5,2.4,(0,0,123.2)))
cover=cover.fuse(cyl(71,3.7,(0,0,119.5)).cut(cyl(69,3.7,(0,0,119.5))))
for yy in [-9,9]:cover=cover.fuse(box(28,yy-4,125.6,8,8,1.4))
cover=drill(cover,[(32,-9),(32,9)],1.7,122,6)
cover=drill(cover,[(40,27),(40,39)],1.7,122,5)
# Open quarter-circle guide keeps the moving wire ends near R33, well below the lid.
def quarter(ro,ri,y,h):
    shell=cyl(ro,h,(0,y,125.5),(0,1,0)).cut(cyl(ri,h,(0,y,125.5),(0,1,0)))
    return shell.intersect(box(0,y-.1,104,22,h+.2,21.6))
cover=cover.fuse(quarter(20.5,12.5,36,3)).fuse(quarter(14,12.5,30,6)).fuse(quarter(20.5,19,30,6))
inlet=cq.Workplane('XY').slot2D(12,7,0).extrude(5).translate((16.5,33,122)).val()
cover=cover.cut(inlet)
# A single raised zero pointer is a visual neutral mark, not a mechanical stop.
cover=cover.fuse(box(-1,64,125.6,2,6,.6))
part('V3_P10_RotatingCableCover',cover,'rotor',orient=lambda s:s.rotate((0,0,0),(1,0,0),180),note='独立打印142mm盖；随臂转动；与固定槽径向/轴向间隙各1mm；曲面导入口')
keeper=box(34,25,126,12,16,3)
for yy in [31.4,33,34.6]:
    wedge=cq.Workplane('YZ').polyline([(yy-.8,125.99),(yy,126.8),(yy+.8,125.99)]).close().extrude(14).translate((33,0,0)).val()
    keeper=keeper.cut(wedge)
keeper=drill(keeper,[(40,27),(40,39)],1.7,125,5)
part('V3_P11_MovingWireKeeper',keeper,'rotor',note='转动端三槽压线片；两颗M3x10；按绝缘皮轻拧，不压伤导线')
clamp=box(-6,59.75,103,12,2,17).cut(cyl(1.7,4,(0,59,114.5),(0,1,0)))
part('V3_P12_FixedWireClamp',clamp,note='固定端夹片，1颗M3x10；调间隙适配约0.8至1.2mm软线')

# Canonical fasteners; thread minor diameters avoid treating cosmetic threads as solid collisions.
part('V3_H01_M3x10',cyl(1.35,10).fuse(cyl(2.75,3,(0,0,-3))),'hardware',False,add=False,note='新增M3x10；外螺纹用小径包络')
part('V3_H02_M3_Nut',hexagon(5.5,2.4).cut(cyl(1.6,3,(0,0,0))),'hardware',False,add=False,note='M3普通螺母，六角对边5.5、厚2.4')
part('V3_H03_ShortM4',cyl(1.8,6).fuse(cyl(3.75,3,(0,0,-3))),'hardware',False,add=False,note='现有短M4示意为6mm；穿过驱动杯3mm，旋入接头约3mm；无需垫套')
part('V3_H04_M4x14',cyl(1.8,14).fuse(cyl(3.75,4,(0,0,-4))),'hardware',False,add=False,note='现有M4x14，固定新增线槽/电机板/支撑上耳')
part('V3_H05_M4_Nut',hexagon(7,3.2).cut(cyl(2,4)),'hardware',False,add=False,note='M4普通螺母，对边7，厚3.2')
for x,y in feet:
    bolt((x,y,12),(0,0,-1),'stationary');nut((x,y,3.6),(0,0,1),'stationary')
    instance('V3_H04_M4x14','stationary',(x,y,99.4),rotation((0,0,-1)))
    instance('V3_H05_M4_Nut','stationary',(x,y,87.8))
for x,y in rootbolts:bolt((x,y,130),(0,0,-1),'rotor');nut((x,y,121.6),(0,0,1),'rotor')
for x,y in sensorfeet:bolt((x,y,133),(0,0,-1),'rotor');nut((x,y,124.6),(0,0,1),'rotor')
for sign in [-1,1]:bolt((16*sign,0,121),(-sign,0,0),'rotor');nut((9.1*sign,0,121),(sign,0,0),'rotor')
bolt((150.5,-3,161),(0,1,0),'pendulum');nut((150.5,3,161),(0,1,0),'pendulum')
instance('V3_H03_ShortM4','rotor',(0,0,127),rotation((0,0,-1)))
for x,y in [(32,-9),(32,9)]:bolt((x,y,130),(0,0,-1),'rotor');nut((x,y,120.8),(0,0,1),'rotor')
for x,y in [(40,27),(40,39)]:bolt((x,y,129),(0,0,-1),'rotor');nut((x,y,120.8),(0,0,1),'rotor')
bolt((0,59.75,114.5),(0,1,0),'stationary');nut((0,65,114.5),(0,1,0),'stationary')

ass=cq.Assembly(name='V3_Furuta_DirectDrive_297')
for row,s in instances:
    color=(.1,.42,.65) if parts[row['name']][0]['printed'] else (.62,.65,.68)
    if row['group']=='pendulum':color=(.98,.49,.08)
    if row['name']=='V3_E03_Existing_M4_Adapter':color=(.8,.69,.35)
    ass.add(s,name=row['label'],color=cq.Color(*color))
ass.export(str(OUT/'V3_倒立摆_完整装配.step'))
for n,(row,s) in parts.items():
    if row['group']=='hardware':row['quantity']=sum(i['name']==n for i,shape in instances)
(OUT/'检查记录'/'零件清单.json').write_text(json.dumps([x[0] for x in parts.values()],ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'检查记录'/'装配实例.json').write_text(json.dumps([x[0] for x in instances],ensure_ascii=False,indent=2),encoding='utf-8')
overlaps=[]
for k,(a,sa) in enumerate(instances):
    aa=sa.BoundingBox()
    for b,sb in instances[k+1:]:
        bb=sb.BoundingBox()
        if any([aa.xmax<=bb.xmin+.001,bb.xmax<=aa.xmin+.001,aa.ymax<=bb.ymin+.001,bb.ymax<=aa.ymin+.001,aa.zmax<=bb.zmin+.001,bb.zmax<=aa.zmin+.001]):continue
        v=sa.intersect(sb).Volume()
        if v>.025:overlaps.append({'a':a['label'],'b':b['label'],'mm3':v})
(OUT/'检查记录'/'干涉检查.json').write_text(json.dumps(overlaps,ensure_ascii=False,indent=2),encoding='utf-8')
print('OVERLAPS',json.dumps(overlaps),flush=True)

