/* Persist only incidents, not the healthy high-rate RX stream. */
(function(root) {
  'use strict';
  const key='bm50-link-incidents-v1';
  const labels={command_timeout:'命令应答超时',fault:'串口会话中断',open_failed:'串口打开或验证失败',
    safe_recovery_start:'保护性停车验证',safe_recovery_verified:'停车重新验证成功（串口未断开）',
    direct_keep_failed:'速度环保活未确认，取消运动',safe_stop_retry:'DISARM 未确认，重发一次停车命令',
    command_rejected:'车端拒绝命令',
    safe_recovery_failed:'停车重新验证失败',keep_ack_missing:'单次 KEEP 确认缺失（仍有运行回传）',
    native_disconnect:'USB/串口设备移除事件',read_error:'串口接收异常',read_ended:'串口接收流结束',
    write_error:'串口写入异常',manual_disconnect:'手动断开',pagehide:'页面退出',
    rx_corruption:'收到损坏字节',rx_unknown_line:'收到未知或损坏的文本行',bridge_closed:'桥接 WebSocket 断开',bridge_status_lost:'桥接报告车端离线',
    bridge_error:'桥接连接失败',bridge_timeout:'桥接命令超时',motion_stopped:'车端报告控制已停止'};
  class Bm50LinkJournal {
    constructor(storage,now=()=>Date.now()) {
      this.storage=storage;this.now=now;this.session=null;this.storageError='';this.incidents=[];
      try {
        const parsed=JSON.parse(storage?.getItem(key)||'null');
        if(parsed?.schema===1 && Array.isArray(parsed.incidents))
          this.incidents=parsed.incidents.slice(-40).filter(x=>x && typeof x.id==='string' && Array.isArray(x.events));
      } catch(_){this.storageError='历史读取失败，仅保留本次记录';}
    }
    start(mode) {
      this.session={id:`${this.now()}-${Math.random().toString(16).slice(2,10)}`,mode,startedAt:this.now()};
    }
    event(event,detail,evidence) {
      if(!labels[event])return;
      if(!this.session)this.start('unknown');
      const previous=this.incidents.at(-1), time=this.now();
      // A damaged UART burst can contain hundreds of chunks. Save its first
      // evidence immediately, then count repeats without synchronous disk I/O.
      if(['rx_corruption','rx_unknown_line'].includes(event) && previous &&
          previous.sessionId===this.session.id && previous.events[0]?.event===event &&
          time-previous.startedAt<1000) {
        previous.updatedAt=time;previous.repeats=(previous.repeats||1)+1;
        previous.lastDetail=detail;return previous;
      }
      const terminal=['manual_disconnect','pagehide','bridge_status_lost','bridge_error','bridge_timeout','motion_stopped','rx_corruption','rx_unknown_line'];
      const grouped=previous && previous.sessionId===this.session.id && !terminal.includes(event) &&
        time-previous.updatedAt<10000 && !['safe_recovery_verified','manual_disconnect','pagehide'].includes(previous.events.at(-1)?.event) &&
        !['rx_corruption','rx_unknown_line','motion_stopped','keep_ack_missing'].includes(previous.events[0]?.event);
      const item=grouped?previous:{id:`${this.session.id}-${time}-${this.incidents.length}`,sessionId:this.session.id,
        mode:this.session.mode,startedAt:time,events:[],rootCause:'未确定；这里记录可观察到的触发条件'};
      if(!grouped)this.incidents.push(item);
      item.updatedAt=time;item.events.push({time,event,reason:labels[event],detail});
      item.events=item.events.slice(-12);
      item.outcome=event==='safe_recovery_verified'?'revalidated':
        ['fault','native_disconnect','read_error','read_ended','write_error','open_failed','bridge_closed','bridge_status_lost','bridge_error'].includes(event)?'disconnected':
        ['manual_disconnect','pagehide'].includes(event)?'manual':item.outcome||'observed';
      if(evidence)item.evidence={...evidence,trace:evidence.trace?.slice(-160),uiTrace:evidence.uiTrace?.slice(-32)};
      this.incidents=this.incidents.slice(-40);
      for(const older of this.incidents.slice(0,-3))delete older.evidence;
      this.persist();
      return item;
    }
    persist() {
      try {
        let value=JSON.stringify({schema:1,incidents:this.incidents});
        if(value.length>220000){for(const item of this.incidents)delete item.evidence;value=JSON.stringify({schema:1,incidents:this.incidents});}
        this.storage?.setItem(key,value);this.storageError='';
      } catch(_){this.storageError='浏览器未能保存历史；请及时导出诊断';}
    }
    snapshot(){return {schema:1,storageError:this.storageError,incidents:JSON.parse(JSON.stringify(this.incidents))};}
  }
  if(typeof module!=='undefined'&&module.exports)module.exports=Bm50LinkJournal;
  else root.Bm50LinkJournal=Bm50LinkJournal;
})(globalThis);
