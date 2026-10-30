"""Nominal planar rotary U-loop. Millimetres, radians, three separately laid soft wires."""
import math,json
from pathlib import Path
RI=33.0;RO=62.25;RB=(RO-RI)/2;RM=(RO+RI)/2
ALPHA_FIXED=math.pi/2
LENGTH=(RI+RO)*math.pi+math.pi*RB
Z_LEVELS=[107.5,109.0,110.5]
def pose(degrees,z=109.0,n=160):
    theta=math.radians(degrees);am=ALPHA_FIXED+theta
    beta=1.5*math.pi+RI/(RI+RO)*theta
    outer=[(RO*math.cos(ALPHA_FIXED+(beta-ALPHA_FIXED)*i/n),RO*math.sin(ALPHA_FIXED+(beta-ALPHA_FIXED)*i/n),z) for i in range(n+1)]
    er=(math.cos(beta),math.sin(beta));et=(-er[1],er[0])
    turn=[((RM+RB*math.cos(math.pi*i/n))*er[0]+RB*math.sin(math.pi*i/n)*et[0],(RM+RB*math.cos(math.pi*i/n))*er[1]+RB*math.sin(math.pi*i/n)*et[1],z) for i in range(1,n+1)]
    inner=[(RI*math.cos(beta+(am-beta)*i/n),RI*math.sin(beta+(am-beta)*i/n),z) for i in range(1,n+1)]
    return {'theta':theta,'alpha_moving':am,'beta':beta,'points':outer+turn+inner,'length':RO*(beta-ALPHA_FIXED)+math.pi*RB+RI*(beta-am)}
def validate():
    checks=[]
    for a in range(-180,181,5):
        p=pose(a);radii=[math.hypot(x,y) for x,y,z in p['points']]
        assert abs(p['length']-LENGTH)<1e-9
        assert min(radii)>=RI-1e-9 and max(radii)<=RO+1e-9
        assert p['beta']>ALPHA_FIXED and p['beta']>p['alpha_moving']
        assert p['beta']-p['alpha_moving']<2*math.pi
        checks.append({'angle_deg':a,'length_mm':p['length'],'bend_position_deg':math.degrees(p['beta']),'radial_range_mm':[min(radii),max(radii)]})
    return {'status':'nominal_kinematics_only','travel_deg':[-180,180],'total_sweep_deg':360,'inner_radius_mm':RI,'outer_radius_mm':RO,'return_bend_radius_mm':RB,'nominal_flexing_length_mm':LENGTH,'wire_od_assumption_mm':[.8,1.2],'z_levels_mm':Z_LEVELS,'samples':checks,'limits':'No cable force, fatigue, gravity, insulation friction, contact or physical test. Wire is NOT a rigid CAD component. Controller or manual travel limits are required; no hard stop is provided.'}
if __name__=='__main__':
    out=Path(__file__).resolve().parents[1];r=validate();(out/'检查记录'/'导线名义运动.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
    # CAD-project schematic, not a manufacturing drawing. Colours identify the three conductors.
    def image(deg):
        paths=[]
        for k,col in enumerate(['#c73737','#2876bd','#333943']):
            pts=pose(deg)['points'];d='M '+' L '.join(f'{x:.3f},{-y:.3f}' for x,y,z in pts)
            paths.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="0.7" transform="translate({(k-1)*.65},0)"/>')
        p=pose(deg);xm=RI*math.cos(p['alpha_moving']);ym=-RI*math.sin(p['alpha_moving'])
        return f'<svg xmlns="http://www.w3.org/2000/svg" width="720" height="760" viewBox="0 0 180 190"><rect width="180" height="190" fill="#f7f9fc"/><text x="90" y="10" text-anchor="middle" font-size="5" font-family="Microsoft YaHei,Arial">{deg:+d}° · 单圈往返名义走线</text><g transform="translate(90,92)"><circle r="68" fill="#e2e9ef"/><circle r="66" fill="#fff"/><circle r="30" fill="#d4dfe8"/><circle r="28" fill="#fff"/>{"".join(paths)}<circle cx="0" cy="-62.25" r="1.8" fill="#26875c"/><circle cx="{xm:.3f}" cy="{ym:.3f}" r="1.8" fill="#e09a21"/></g><text x="90" y="173" text-anchor="middle" font-size="4.5" font-family="Microsoft YaHei,Arial">绿：固定端　橙：随悬臂转动端　U形回弯随动</text><text x="90" y="182" text-anchor="middle" font-size="4" font-family="Microsoft YaHei,Arial">示意为三根线的投影，实物在不同高度平行布置；尚需试装。</text></svg>'
    for a in [-180,0,180]:(out/'预览'/f'走线示意_{a:+d}度.svg').write_text(image(a),encoding='utf-8')
    print('nominal flexing length',round(LENGTH,3),'mm; bend radius',RB,'mm; 73 poses passed')
