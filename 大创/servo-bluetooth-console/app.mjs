import {BleSession} from './ble.mjs';
import {motionValues,FIRMWARE} from './protocol.mjs';
const $=id=>document.getElementById(id);
let selectedMode='M',direction=1,connected=false,connecting=false,busy=false,deviceMode='PAUSE';
let operationId=0;
const lines=[];
function log(text){
  lines.push(`${new Date().toLocaleTimeString('zh-CN',{hour12:false})}  ${text}`);
  if(lines.length>80)lines.shift();$('log').textContent=lines.join('\n');$('log').scrollTop=$('log').scrollHeight;
}
function message(text,error=false){$('message').textContent=text;$('message').style.color=error?'#ef6868':'';}
function render(){
  $('connect').disabled=connecting||connected;$('connect').textContent=connecting?'正在连接…':'连接蓝牙';
  $('disconnect').disabled=!connected;$('profile').disabled=connecting||connected;
  $('stop').disabled=!connected;$('execute').disabled=!connected||busy||deviceMode==='FAULT';
  $('connectionBadge').classList.toggle('connected',connected);
  $('connectionText').textContent=connected?'蓝牙已连接':connecting?'等待连接':'尚未连接';
  $('execute').textContent=busy?'等待设备应答…':selectedMode==='M'?'执行转动':selectedMode==='C'?'开始连续旋转':'开始往复运动';
  $('angle').disabled=selectedMode==='C';
  document.querySelectorAll('[data-angle]').forEach(b=>b.disabled=selectedMode==='C');
  document.querySelectorAll('[data-mode]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.mode===selectedMode)));
  document.querySelectorAll('[data-dir]').forEach(b=>b.setAttribute('aria-pressed',String(+b.dataset.dir===direction)));
  $('modeHint').textContent=selectedMode==='M'?'从当前位置相对转动指定角度，不自动重复发送。':selectedMode==='C'?'持续沿选定方向旋转，按“停止运动”终止。角度输入在此模式下不生效。':'沿选定方向转动指定角度，再反向相同角度，循环往复。';
  const angle=Number($('angle').value);
  $('targetAngle').textContent=selectedMode==='C'?'∞':`${Number.isFinite(angle)?angle:'—'}°`;
  $('targetLabel').textContent=`${selectedMode==='C'?'连续旋转':'本次目标'} · ${direction>0?'正向':'反向'}`;
  $('needle').style.transform=`rotate(${Number.isFinite(angle)?angle*direction:0}deg)`;
}
const names={PAUSE:'已停止',MOVE:'转动中',RUN:'连续转动',AUTO:'往复运动',FAULT:'舵机异常'};
const session=new BleSession({
  onLog:log,
  onStatus:state=>{
    deviceMode=state.mode;$('mode').textContent=names[state.mode];$('commands').textContent=state.commands;
    $('errors').textContent=state.errors;$('presses').textContent=state.presses;$('firmware').textContent=state.version;
    $('mode').style.color=state.mode==='FAULT'?'#ef6868':state.mode==='PAUSE'?'':'#5dd8df';
    if(connected && state.mode==='FAULT')message('舵机执行异常：通信、实际转速或方向检查未通过，已请求停止。请检查设备后重新执行。',true);
    render();
  },
  onDisconnect:()=>{
    connected=false;busy=false;$('deviceName').textContent='连接你的舵机';
    $('connectionHint').textContent='连接已断开。重新连接后会先停止并核对固件。';
    $('mode').textContent='连接已断开';$('firmware').textContent='等待重新核对';
    message('蓝牙连接已断开；远程运动将由控制器超时停止。',true);log('蓝牙连接已断开');render();
  },
});

$('connect').addEventListener('click',async()=>{
  connecting=true;render();message('请在浏览器弹窗中选择你的蓝牙模块。');
  try{
    const name=await session.connect($('profile').value);connected=true;
    $('deviceName').textContent=name;$('connectionHint').textContent='固件已核对。可设置角度或选择连续旋转。';
    log(`已连接 ${name}，固件匹配 ${FIRMWARE}`);message(deviceMode==='FAULT'?'蓝牙已连接，舵机当前异常；请检查舵机供电与接线。':'连接成功，控制器已进入停止状态。',deviceMode==='FAULT');
  }catch(error){message(`连接失败：${error.message}`,true);log(`连接失败：${error.message}`);}
  finally{connecting=false;render();}
});
$('disconnect').addEventListener('click',()=>session.disconnect());

async function stop(){
  if(!connected)return;
  const operation=++operationId;
  busy=true;render();
  try{await session.command('S');message('停止命令已确认。');log('控制器已确认停止');}
  catch(error){message(`停止未确认：${error.message}`,true);log(`停止未确认：${error.message}`);}
  finally{if(operation===operationId)busy=false;render();}
}
$('stop').addEventListener('click',stop);
$('execute').addEventListener('click',async()=>{
  if(!connected||busy)return;
  let angle,speed;
  try{[angle,speed]=motionValues(selectedMode==='C'?1:$('angle').value,$('speed').value,direction);}
  catch(error){message(error.message,true);return;}
  const operation=++operationId;
  busy=true;render();
  try{
    await session.command(selectedMode,selectedMode==='C'?direction:angle,speed);
    const action=selectedMode==='C'?`${direction>0?'正向':'反向'}连续旋转`:selectedMode==='A'?`往复 ${Math.abs(angle)/10}°`:`${direction>0?'正向':'反向'}转动 ${Math.abs(angle)/10}°`;
    message(`控制器已接受：${action}。`);log(`已确认 ${action}，${$('speed').value}°/s`);
  }catch(error){
    if(error.name==='AbortError'){log('运动请求已由停止操作取消');return;}
    message(`命令未确认：${error.message}。已请求停止，请确认设备状态。`,true);log(`命令异常：${error.message}`);
    try{await session.command('S');}catch{/* MCU heartbeat lease is the fallback. */}
  }finally{if(operation===operationId)busy=false;render();}
});
document.querySelectorAll('[data-mode]').forEach(button=>button.addEventListener('click',()=>{selectedMode=button.dataset.mode;render();}));
document.querySelectorAll('[data-dir]').forEach(button=>button.addEventListener('click',()=>{direction=+button.dataset.dir;render();}));
document.querySelectorAll('[data-angle]').forEach(button=>button.addEventListener('click',()=>{$('angle').value=button.dataset.angle;render();}));
$('angle').addEventListener('input',render);
$('speed').addEventListener('input',()=>{$('speedSlider').value=$('speed').value;});
$('speedSlider').addEventListener('input',()=>{$('speed').value=$('speedSlider').value;});
$('clearLog').addEventListener('click',()=>{lines.length=0;$('log').textContent='记录已清空。';});
window.addEventListener('keydown',event=>{if(event.code==='Space'&&!['INPUT','SELECT','TEXTAREA'].includes(document.activeElement?.tagName)){event.preventDefault();void stop();}});
document.addEventListener('visibilitychange',()=>{if(document.hidden&&connected)void stop();});
window.addEventListener('pagehide',()=>{void session.disconnect();});
if(!window.isSecureContext||!navigator.bluetooth)message('需要支持 Web Bluetooth 的桌面 Chrome / Edge，请通过 localhost 打开本页。',true);
render();
