/* Direct Web Serial transport. No WebSocket, Python process or motion retry. */
(function(root) {
  'use strict';
  const queries = {PING:'PONG',VER:'VER',VERSION:'VER',UART:'UART',LINK:'LINK',LINKQ:'LINKQ',
    STACK:'STACK',CAPS:'CAPS',SPEEDSTATUS:'TEL',STATUS:'TEL',SPEEDLIMIT:'SPEEDLIMIT',
    PIDGET:'PID',YAWGET:'YAWPID',HEADGET:'HEADPID',ANGLESTATUS:'ATEL',PS2STATUS:'PS2',
    PS2RAW:'PS2RAW',TOF:'TOF',TOFINFO:'TOFINFO',WLOGINFO:'WLOG,INFO',ENC:'ENC',BAT:'BAT',WHO:'WHO',HELP:'CMD','?':'CMD'};
  function expected(command) {
    const parts = command.split(','), name = parts[0];
    if (/^(TEL|TOF|SPEED),[01]$/.test(command)) return `ACK,${command}`;
    if (name === 'HLOG' || name === 'WLOG') {
      if (parts[1] === 'READ') return `${name},END,${Number(parts[2])}`;
      return `${name},${parts[1]}`;
    }
    if (queries[name]) return queries[name];
    if (name === 'SPEED') return 'ACK,SPEED,LIMIT';
    if (name === 'CAL' || name === 'GYROCAL') return 'ACK,CAL';
    if (name === 'ZERO' || name === 'YAWZERO') return 'ACK,ZERO';
    if (/^[ABCD][+-]$/.test(command)) return 'ACK,JOG';
    return `ACK,${name}`;
  }
  const matches = (line, prefix) => line === prefix || line.startsWith(prefix + ',');
  const cancelled = () => Object.assign(new Error('cancelled by STOP/disconnection'), {cancelled:true});

  class Bm50SerialTransport {
    constructor({onLine=()=>{},onDisconnect=()=>{},onWarning=()=>{},onRecovering=()=>{},onRecovered=()=>{},onEvent=()=>{},clock=()=>performance.now()}={}) {
      this.onEvent=onEvent; this.onLine=onLine; this.onDisconnect=onDisconnect; this.onWarning=onWarning; this.clock=clock;
      this.onRecovering=onRecovering;this.onRecovered=onRecovered;this.recovering=false;
      this.queue=[]; this.pending=null; this.epoch=0; this.buffer=''; this.discard=false;
      this.opened=false; this.verified=false; this.fault=null; this.pumping=false;
      this.lastReplyAt=0;
      this.lastRxAt=null; this.motionReport=null; this.heartbeatAckMisses=0;
      this.writeChain=Promise.resolve(); this.trace=[]; this.traceDropped=0;
      this.stats={rxBytes:0,rxLines:0,badBytes:0,timeouts:0,txCommands:0,keepAckMisses:0,stopRecoveries:0};
    }
    record(event, detail) {
      if (this.trace.length >= 2048) { this.trace.shift(); this.traceDropped++; }
      const row={hostMs:this.clock(),event,detail};
      this.trace.push(row);
      if(!['rx','tx','write_done','reply'].includes(event)) {
        try {this.onEvent(event,detail);}catch(_){} // Diagnostics cannot interrupt control.
      }
    }
    diagnostic() { return {mode:'direct_web_serial',baud:115200,port:this.info,
      verified:this.verified,recovering:this.recovering,fault:this.fault?.message,stats:{...this.stats},
      heartbeatAckMisses:this.heartbeatAckMisses,lastRxAt:this.lastRxAt,motionReport:this.motionReport,
      traceDropped:this.traceDropped,trace:this.trace.slice()}; }
    isOpen() { return this.opened && !this.closing && !this.fault; }
    async open(port) {
      if (this.opened || this.closing) throw new Error('串口会话不能重复打开');
      this.port=port; this.info=port.getInfo?.();
      this.nativeDisconnect=()=>{this.record('native_disconnect',this.info);this.fail(new Error('USB/串口设备已移除'));};
      port.addEventListener?.('disconnect',this.nativeDisconnect);
      try {
        await port.open({baudRate:115200,dataBits:8,stopBits:1,parity:'none',flowControl:'none',bufferSize:4096});
        this.opened=true; this.writer=port.writable.getWriter(); this.reader=port.readable.getReader();
        this.readTask=this.readLoop();
        for (const command of ['STOP','DISARM','TEL,0','TOF,0','PING']) await this.command(command);
        this.verified=true; this.record('verified',this.info);
        this.monitor=setInterval(()=>{
          if(this.isOpen() && this.verified && !this.heartbeatAckMisses && !this.pending && !this.queue.length && this.clock()-this.lastReplyAt>=1000)
            void this.command('PING').catch(()=>{});
        },1000);
      } catch(error) { this.record('open_failed',{name:error.name,message:error.message});await this.close(false); throw error; }
    }
    async readLoop() {
      try {
        while (this.opened) {
          const {value,done}=await this.reader.read();
          if (done) { if (!this.closing) {this.record('read_ended',{});this.fail(new Error('串口接收已结束'));} break; }
          if (value) this.consume(value);
        }
      } catch(error) { if (!this.closing) {this.record('read_error',{name:error.name,message:error.message});this.fail(error);} }
      finally { try {this.reader?.releaseLock();}catch(_){} }
    }
    consume(bytes) {
      const badBefore=this.stats.badBytes;
      this.stats.rxBytes+=bytes.length;
      if(bytes.length)this.lastRxAt=this.clock();
      this.record('rx',Array.from(bytes,b=>b.toString(16).padStart(2,'0')).join(''));
      for (const byte of bytes) {
        if (byte===13) continue;
        if (byte===10) {
          const line=this.buffer; this.buffer='';
          if (this.discard) { this.discard=false; continue; }
          if (!line) continue;
          this.stats.rxLines++;
          if(!/^(ACK|ERR|PONG|VER|UART|LINK|LINKQ|STACK|CAPS|TEL|ATEL|TOF|TOFINFO|PID|YAWPID|HEADPID|PS2|PS2RAW|HLOG|WLOG|ENC|BAT|WHO|CMD|IMU|DRV)(,|$)/.test(line))
          {
            this.record('rx_unknown_line',{line:line.slice(0,160),pending:this.pending?.entry.command});
            this.onWarning(`串口收到未知或损坏行：${line.slice(0,100)}`);
          }
          if(line.startsWith('ATEL,')) {
            const p=line.split(',');
            if(p.length===14 && p.slice(1).every(x=>/^-?\d+$/.test(x)&&Number.isSafeInteger(Number(x))) &&
                ['0','1','2'].includes(p[2]) && ['0','1'].includes(p[3]) && ['0','1'].includes(p[12]))
              this.motionReport={receivedAt:this.clock(),tick:Number(p[1]),mode:Number(p[2]),imuReady:p[3]==='1',enabled:p[12]==='1'};
          }
          this.onLine(line); // State and all log rows reach the page before ACK completion.
          const pending=this.pending;
          if (pending && pending.epoch===this.epoch) {
            if (line.startsWith('ERR,')) pending.reject(new Error(line));
            else if (matches(line,pending.expected)) {
              this.lastReplyAt=this.clock();
              if(pending.entry.command==='KEEP')this.heartbeatAckMisses=0;
              if(/^(MOVE|YAW|HEAD|HEADMOVE|TRACKMOVE),/.test(pending.entry.command))
                this.motionReport={receivedAt:this.clock(),source:line,mode:/^(MOVE|YAW),/.test(pending.entry.command)?1:2,imuReady:true,enabled:true};
              if(['STOP','DISARM','ARM','SPEED,0'].includes(pending.entry.command))
                this.motionReport={receivedAt:this.clock(),source:line,mode:0,imuReady:false,enabled:false};
              this.record('reply',{command:pending.entry.command,line,elapsedMs:this.clock()-pending.entry.startedAt});
              pending.resolve(line);
            }
          }
          continue;
        }
        if (byte<32 || byte>126 || this.buffer.length>=4096) {
          this.stats.badBytes++; this.discard=true; this.buffer='';
        }
        if (!this.discard) this.buffer+=String.fromCharCode(byte);
      }
      if(this.stats.badBytes>badBefore) {
        this.record('rx_corruption',{badBytes:this.stats.badBytes-badBefore,pending:this.pending?.entry.command});
        this.onWarning('串口收到损坏字节或超长行；已保留原始数据，丢弃该行');
      }
    }
    command(text, timeout) {
      const command=String(text).trim().toUpperCase();
      if (!/^[\x20-\x7e]{1,63}$/.test(command)) return Promise.reject(new Error('命令必须是1..63字节ASCII单行'));
      if (!this.opened || this.fault) return Promise.reject(new Error('串口未就绪'));
      if(this.recovering)return Promise.reject(new Error('正在验证停车与DISARM；旧运动已取消，请等待验证完成'));
      const urgent=command==='STOP'||command==='DISARM';
      if (urgent) {
        const same=this.pending?.entry.command===command ? this.pending.entry : this.queue.find(e=>e.command===command);
        if (same) return same.promise;
        this.epoch++; this.pending?.reject(cancelled());
        this.queue.splice(0).forEach(e=>e.reject(cancelled()));
      }
      const realtime=/^(MOVE|DRV|HEADMOVE|TRACKMOVE),/.test(command);
      if (realtime) {
        this.queue=this.queue.filter(e=>{if(e.realtime){e.reject(cancelled());return false;}return true;});
      }
      if (!urgent && this.queue.length>=64) return Promise.reject(new Error('串口命令队列已满'));
      const entry={command,realtime,epoch:this.epoch,enqueued:this.clock(),
        timeout:timeout??(command==='DKEEP'?200:command==='KEEP'?600:/^(HLOG|WLOG),READ,/.test(command)?20000:3000)};
      entry.promise=new Promise((resolve,reject)=>{entry.resolve=resolve;entry.reject=reject;});
      if (urgent) this.queue.unshift(entry); else this.queue.push(entry);
      void this.pump(); return entry.promise;
    }
    async pump() {
      if (this.pumping) return;
      this.pumping=true;
      try {
        while(this.queue.length && this.opened && !this.fault) {
          const entry=this.queue.shift();
          try {
            if(entry.epoch!==this.epoch) throw cancelled();
            const urgent=entry.command==='STOP'||entry.command==='DISARM';
            if(!urgent && this.clock()-entry.enqueued>(entry.realtime?250:3000)) throw new Error('拒绝执行已过期的排队命令');
            entry.resolve(await this.execute(entry));
          } catch(error) {
            if(error?.stopAckMissing && this.opened && !this.closing) {
              this.recoveryTask=this.recoverStoppedLink();
              entry.reject(error);await this.recoveryTask;
            } else entry.reject(error);
          }
        }
      } finally {this.pumping=false;}
    }
    async execute(entry) {
      let timer;
      entry.startedAt=this.clock();
      const ack=new Promise((resolve,reject)=>{
        this.pending={entry,epoch:entry.epoch,expected:expected(entry.command),resolve,reject};
        timer=setTimeout(()=>{
          this.stats.timeouts++;
          const rxAgeMs=this.lastRxAt===null?null:this.clock()-this.lastRxAt;
          this.record('command_timeout',{command:entry.command,rxAgeMs,writeDone:!!entry.writeDone,motionReport:this.motionReport});
          // Waiting 3 s for KEEP's ACK starves the MCU's 1.5 s watchdog.
          // Permit ONE missed ACK only with a completed write and recent active
          // ATEL plus ongoing RX. The next scheduled KEEP cannot restart mode=0.
          const state=this.motionReport;
          if(entry.command==='STOP' && entry.writeDone && this.verified && !this.closing &&
              rxAgeMs!==null && rxAgeMs<500 && state && this.clock()-state.receivedAt<=1000 &&
              state.mode===0 && !state.enabled) {
            const error=Object.assign(new Error('STOP 确认丢失；正在重新验证DISARM和停车状态，旧运动已取消'),{stopAckMissing:true});
            this.record('stop_ack_missing',error.message);reject(error);return;
          }
          if(entry.command==='KEEP' && entry.writeDone && this.verified && !this.closing &&
              !this.heartbeatAckMisses && rxAgeMs!==null && rxAgeMs<=500 &&
              this.lastRxAt>=entry.startedAt && state && this.clock()-state.receivedAt<=1000 &&
              state.mode && state.imuReady && state.enabled) {
            this.heartbeatAckMisses++;this.stats.keepAckMisses++;
            const error=Object.assign(new Error('KEEP 确认丢失一次；车端仍回传运行状态，等待下一次保活'),{heartbeatAckMissing:true});
            this.record('keep_ack_missing',error.message);reject(error);return;
          }
          const error=new Error(`车端应答超时：${entry.command}；${rxAgeMs!==null&&rxAgeMs<500?'仍有回传，但该命令未确认':'未获得可靠车端应答'}`);
          reject(error);this.fail(error);
        },entry.timeout);
      });
      // Observe rejection even if a stalled write outlives the ACK deadline.
      ack.catch(()=>{});
      this.writeChain=this.writeChain.catch(()=>{}).then(async()=>{
        if(entry.epoch!==this.epoch || !this.opened) throw cancelled();
        this.record('tx',entry.command); this.stats.txCommands++;
        await this.writer.write(new TextEncoder().encode(`\n${entry.command}\n`));
        entry.writeDone=true;this.record('write_done',entry.command);
      }).catch(error=>{
        if(!error?.cancelled){this.record('write_error',{name:error?.name,message:error?.message||'串口写入失败'});this.fail(error);}
        throw this.fault||error;
      });
      try {
        // A cancelled ACK must unblock STOP even if an earlier write is slow.
        await Promise.race([this.writeChain,ack.then(()=>{})]);
        return await ack;
      } finally {clearTimeout(timer);if(this.pending?.entry===entry)this.pending=null;}
    }
    async recoverStoppedLink() {
      this.recovering=true;this.verified=false;this.epoch++;
      const epoch=this.epoch;
      this.queue.splice(0).forEach(entry=>entry.reject(cancelled()));
      this.record('safe_recovery_start','STOP ACK missing; DISARM / ANGLESTATUS / PING only');
      try {
        this.onRecovering();
        for(const command of ['DISARM','ANGLESTATUS','PING']) {
          if(this.closing || !this.opened || this.epoch!==epoch)throw cancelled();
          const reply=await this.execute({command,epoch,timeout:1000});
          if(command==='ANGLESTATUS') {
            const p=reply.split(',');
            if(p.length!==14 || !p.slice(1).every(x=>/^-?\d+$/.test(x)&&Number.isSafeInteger(Number(x))) ||
                p[2]!=='0' || p[12]!=='0')throw new Error('停车回读未确认，拒绝恢复控制');
          }
        }
        if(this.closing || !this.opened || this.epoch!==epoch)throw cancelled();
        this.heartbeatAckMisses=0;this.verified=true;this.recovering=false;
        this.stats.stopRecoveries++;this.record('safe_recovery_verified','DISARM ACK + stopped ATEL + PONG; no motion replay');
        this.onRecovered();
      } catch(error) {this.record('safe_recovery_failed',{name:error.name,message:error.message});this.fail(error);}
      finally {this.recovering=false;}
    }
    fail(error) {
      if(this.fault || this.closing) return;
      if(!error || typeof error.message!=='string')error=new Error(String(error??'串口流读写失败'));
      this.fault=error;this.verified=false;this.epoch++;
      this.record('fault',{name:error.name,message:error.message,pending:this.pending?.entry.command});this.pending?.reject(error);
      this.queue.splice(0).forEach(e=>e.reject(error));
      this.onDisconnect(error);void this.close(false);
    }
    close(safeStop=true) {
      if(this.closing)return this.closing;
      this.closing=Promise.resolve().then(async()=>{
        clearInterval(this.monitor);
        this.port?.removeEventListener?.('disconnect',this.nativeDisconnect);
        if(safeStop && this.opened && !this.fault) {
          try {await this.command('STOP',1000);await this.command('DISARM',1000);}
          catch(error){this.record('unverified_stop',error.message);}
        }
        this.opened=false;this.verified=false;this.epoch++;
        this.pending?.reject(cancelled());this.queue.splice(0).forEach(e=>e.reject(cancelled()));
        try {await this.reader?.cancel();}catch(_){}
        await this.readTask;
        try {await this.writer?.abort();}catch(_){}
        try {this.writer?.releaseLock();}catch(_){}
        try {await this.port?.close();}catch(error){this.record('close_error',error.message);}
      });return this.closing;
    }
  }
  Bm50SerialTransport.expected=expected;
  if(typeof module!=='undefined'&&module.exports)module.exports=Bm50SerialTransport;
  else root.Bm50SerialTransport=Bm50SerialTransport;
})(globalThis);
