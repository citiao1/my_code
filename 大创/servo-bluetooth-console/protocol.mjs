export const FIRMWARE = 'SERVO BLE R7';
export const PROFILES = {
  type1: {service:0xffe0, write:0xffe1, notify:0xffe1},
  type2: {service:0xffe0, write:0xffe2, notify:0xffe1},
};
export function checksum(text) {
  return [...new TextEncoder().encode(text)].reduce((a,b)=>a^b,0).toString(16).padStart(2,'0').toUpperCase();
}
export function request(id, op, ...args) {
  if (!Number.isInteger(id) || id<1 || id>65535 || !['V','K','S','M','C','A'].includes(op)) throw new Error('无效命令');
  if ('MCA'.includes(op)) {
    if (args.length!==2 || !args.every(Number.isInteger) || args[1]<100 || args[1]>1500) throw new Error('无效运动参数');
    if (op==='C' ? ![-1,1].includes(args[0]) : !args[0] || Math.abs(args[0])>7200) throw new Error('角度或方向超出范围');
  } else if (args.length) throw new Error('多余参数');
  const body=[id,op,...args].join(',');
  return `@${body}*${checksum(body)}\n`;
}
export function decode(line) {
  const match=/^!([^*]+)\*([0-9a-f]{2})$/i.exec(line.replace(/\r$/,''));
  if (!match || checksum(match[1])!==match[2].toUpperCase()) throw new Error('应答校验失败');
  const [id,result,...data]=match[1].split(',');
  if (!/^\d+$/.test(id) || +id<1 || +id>65535 || !['OK','ERR'].includes(result)) throw new Error('无效应答');
  if (result==='ERR') {
    if (data.length!==1 || !/^[A-Z_]+$/.test(data[0])) throw new Error('无效错误帧');
    return {id:+id,error:data[0]};
  }
  if(data.length===5 && data[0]!==FIRMWARE){const error=new Error(`固件版本不匹配：需要 ${FIRMWARE}`);error.code='VERSION';error.id=+id;throw error;}
  if (data.length!==5 || !['PAUSE','MOVE','RUN','AUTO','FAULT'].includes(data[1]) ||
      data.slice(2).some(v=>!/^\d+$/.test(v) || !Number.isSafeInteger(+v))) throw new Error('固件版本或状态格式不匹配');
  return {id:+id,version:data[0],mode:data[1],errors:+data[2],commands:+data[3],presses:+data[4]};
}

export class ServoClient {
  constructor(write,{timeout=2500,onStatus=()=>{},onLog=()=>{}}={}) {
    this.write=write; this.timeout=timeout; this.onStatus=onStatus; this.onLog=onLog;
    this.id=1+Math.floor(Math.random()*40000); this.pending=new Map(); this.buffer=''; this.decoder=new TextDecoder();
    this.closed=false; this.generation=0;
  }
  get busy(){ return this.pending.size>0; }
  feed(value) {
    const bytes=value instanceof DataView ? new Uint8Array(value.buffer,value.byteOffset,value.byteLength) : value;
    this.buffer+=typeof bytes==='string'?bytes:this.decoder.decode(bytes,{stream:true});
    if(this.buffer.length>2048){this.buffer='';this.onLog('接收缓冲溢出，等待新应答');return;}
    let at;
    while((at=this.buffer.indexOf('\n'))>=0){
      const line=this.buffer.slice(0,at);this.buffer=this.buffer.slice(at+1);
      let message;
      try{message=decode(line);}catch(error){
        if(error.code==='VERSION'){const p=this.pending.get(error.id);if(p){clearTimeout(p.timer);this.pending.delete(error.id);p.reject(error);}}
        this.onLog(error.message);continue;
      }
      const pending=this.pending.get(message.id);
      if(!pending)continue; // Ignore stale acknowledgements from an earlier command/session.
      clearTimeout(pending.timer);this.pending.delete(message.id);
      if(message.error)pending.reject(new Error(message.error));
      else{this.onStatus(message);pending.resolve(message);}
    }
  }
  command(op,...args){
    if(this.closed)return Promise.reject(new Error('连接已断开'));
    if(this.busy && op!=='S')return Promise.reject(new Error('正在等待上一条命令应答'));
    if(op==='S')this.cancelPending('被停止命令取消');
    this.id=this.id%65535+1;
    const id=this.id, wire=request(id,op,...args), generation=this.generation;
    return new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>{this.pending.delete(id);reject(new Error('设备应答超时；运动命令未自动重发'));},this.timeout);
      this.pending.set(id,{resolve,reject,timer});
      if(op!=='K')this.onLog(`发送 ${op==='M'?'定角度':op==='C'?'连续旋转':op==='S'?'停止':op==='A'?'往复运动':op}${args.length?' '+args.join(','):''}`);
      Promise.resolve().then(()=>{if(generation!==this.generation || this.closed)throw new Error('连接已切换');return this.write(new TextEncoder().encode(wire));})
        .catch(error=>{const p=this.pending.get(id);if(p){clearTimeout(p.timer);this.pending.delete(id);p.reject(error);}});
    });
  }
  cancelPending(reason){for(const p of this.pending.values()){clearTimeout(p.timer);const error=new Error(reason);if(reason==='被停止命令取消')error.name='AbortError';p.reject(error);}this.pending.clear();}
  close(){this.closed=true;this.generation++;this.cancelPending('连接已断开');this.buffer='';}
}

export function motionValues(angle, degreesPerSecond, direction){
  const a=Number(angle),speed=Number(degreesPerSecond);
  if(!Number.isFinite(a)||a<0.1||a>720||!Number.isFinite(speed)||speed<10||speed>120||![-1,1].includes(direction))throw new Error('角度需为 0.1–720°，速度需为 10–120°/s');
  return [Math.round(a*10)*direction,Math.round(speed*4096/360)];
}
