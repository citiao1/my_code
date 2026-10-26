import {ServoClient,PROFILES} from './protocol.mjs';

export class BleSession {
  constructor({onStatus=()=>{},onLog=()=>{},onDisconnect=()=>{}}={}){
    this.onStatus=onStatus;this.onLog=onLog;this.onDisconnect=onDisconnect;
    this.device=null;this.client=null;this.mode='PAUSE';this.verified=false;this.closing=false;this.token=0;this.chain=Promise.resolve();
    this.notification=event=>{
      const value=event.target.value;
      if(!this.verified)this.onLog(`收到蓝牙通知 ${value.byteLength} 字节`);
      this.client?.feed(value);
    };
    this.disconnected=()=>this.release(true);
  }
  get connected(){return !!this.device?.gatt?.connected && this.verified;}
  async connect(profileName){
    if(!navigator.bluetooth)throw new Error('请使用支持 Web Bluetooth 的桌面 Chrome 或 Edge，并从 localhost 打开页面');
    const profile=PROFILES[profileName];if(!profile)throw new Error('请选择蓝牙模块类型');
    const token=++this.token;
    // requestDevice runs directly from the user's connection-button click.
    let device;
    let remembered;
    try{remembered=localStorage.getItem('servo-ble-device');}catch{/* Storage can be unavailable. */}
    if(remembered && navigator.bluetooth.getDevices){
      const permitted=await navigator.bluetooth.getDevices();
      device=permitted.find(item=>item.id===remembered);
    }
    if(!device)device=await navigator.bluetooth.requestDevice({filters:[{services:[profile.service]}]});
    if(token!==this.token)throw new Error('连接已取消');
    try{localStorage.setItem('servo-ble-device',device.id);}catch{/* Connection does not require storage. */}
    this.device=device;this.closing=false;this.chain=Promise.resolve();
    device.addEventListener('gattserverdisconnected',this.disconnected);
    try{
      const server=await device.gatt.connect();
      this.onLog(`蓝牙已连接 ${device.name||'BLE 模块'}，正在核对固件`);
      const service=await server.getPrimaryService(profile.service);
      this.notify=await service.getCharacteristic(profile.notify);
      this.writeCharacteristic=profile.write===profile.notify?this.notify:await service.getCharacteristic(profile.write);
      if(!this.writeCharacteristic.properties.write&&!this.writeCharacteristic.properties.writeWithoutResponse)throw new Error('当前模块类型不支持写入，请尝试另一种模块类型');
      this.client=new ServoClient(bytes=>this.write(bytes,token),{
        onLog:this.onLog,onStatus:state=>{
          if(state.mode!==this.mode || state.errors!==this.lastErrors){
            const names={PAUSE:'已停止',MOVE:'定角度转动',RUN:'连续旋转',AUTO:'往复运动',FAULT:'舵机异常'};
            this.onLog(`设备状态：${names[state.mode]}，累计舵机错误 ${state.errors}`);
          }
          this.mode=state.mode;this.lastErrors=state.errors;this.onStatus(state);
        },
      });
      this.notify.addEventListener('characteristicvaluechanged',this.notification);
      await this.notify.startNotifications();
      this.onLog(`通知 FFE1 已启用，写入 ${profile.write===0xffe1?'FFE1':'FFE2'}`);
      await this.client.command('V');
      if(token!==this.token)throw new Error('连接已取消');
      this.verified=true;
      // Take over in a stopped state. A servo-side fault can still be inspected.
      try{await this.client.command('S');}catch(error){this.onLog(`停止应答：${error.message}`);}
      this.timer=setInterval(()=>this.heartbeat(),700);
      return device.name||'BLE 串口模块';
    }catch(error){this.release(false);throw error;}
  }
  write(bytes,token){
    const task=this.chain.then(async()=>{
      for(let offset=0;offset<bytes.length;offset+=20){
        if(token!==this.token||!this.device?.gatt?.connected)throw new Error('连接已断开');
        const chunk=bytes.slice(offset,offset+20),characteristic=this.writeCharacteristic;
        if(characteristic.properties.writeWithoutResponse)await characteristic.writeValueWithoutResponse(chunk);
        else if(characteristic.writeValueWithResponse)await characteristic.writeValueWithResponse(chunk);
        else await characteristic.writeValue(chunk);
      }
    });
    this.chain=task.catch(()=>{});return task;
  }
  async command(op,...args){
    if(!this.connected)return Promise.reject(new Error('尚未完成蓝牙连接和固件核对'));
    if(op!=='S' && this.heartbeatPending){
      try{await this.heartbeatPending;}catch(error){if(error.name!=='AbortError')throw error;}
      if(!this.connected)throw new Error('连接已断开');
    }
    return this.client.command(op,...args);
  }
  async heartbeat(){
    if(!this.connected||this.client.busy||this.closing)return;
    const pending=this.client.command('K');this.heartbeatPending=pending;
    try{await pending;}
    catch(error){if(error.name==='AbortError'||!this.connected)return;this.onLog(`状态链路异常：${error.message}`);await this.disconnect();}
    finally{if(this.heartbeatPending===pending)this.heartbeatPending=null;}
  }
  async disconnect(){
    if(this.closing)return;
    this.closing=true;
    clearInterval(this.timer);
    try{if(this.connected)await this.client.command('S');}catch(error){this.onLog(`停止未确认：${error.message}，控制器将按断联超时停止`);}
    this.release(true);
  }
  release(announce){
    ++this.token;clearInterval(this.timer);this.client?.close();
    this.notify?.removeEventListener('characteristicvaluechanged',this.notification);
    const device=this.device;
    device?.removeEventListener('gattserverdisconnected',this.disconnected);
    if(device?.gatt?.connected)device.gatt.disconnect();
    this.client=null;this.device=null;this.notify=null;this.writeCharacteristic=null;
    this.verified=false;this.closing=false;this.heartbeatPending=null;
    if(announce)this.onDisconnect();
  }
}
