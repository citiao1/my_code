/* R20 physical A/B/C/D wheel log. Units are explicit; no interpolation. */
(function (root) {
  'use strict';
  function row(line) {
    const p = line.trim().split(',');
    if (p.length !== 30 || p[0] !== 'WLOG' || p[1] !== 'DATA' ||
        !p.slice(2).every(x => /^-?\d+$/.test(x))) throw new Error('WLOG 数据格式错误');
    const v = p.slice(2).map(Number);
    if (!v.every(Number.isSafeInteger) || v.slice(0, 3).some(x => x < 0 || x > 0xffffffff) ||
        ![v[3], v[4], v[23], v[26], v[27]].every(x => x === 0 || x === 1) ||
        ![0, 1, 2].includes(v[5]) || v[22] < 0 || v[22] > 15 ||
        v[24] < 0 || v[24] > 65535 ||
        (v[26] ? v[25] < 1 || v[25] > 4000 : v[25] !== -1))
      throw new Error('WLOG 状态或范围无效');
    for (let i = 6; i < 22; i += 4) {
      if (Math.abs(v[i]) > 200 || Math.abs(v[i+1]) > 2147483647 ||
          Math.abs(v[i+2]) > 2147483647 || Math.abs(v[i+3]) > 60000)
        throw new Error('WLOG 轮速或输出越界');
    }
    return v;
  }
  function validate(rows) {
    if (!rows.length) throw new Error('WLOG 为空');
    rows.forEach((v, i) => {
      if (v[0] !== i || ((v[2] - v[1] * 1000) >>> 0) >= 1000)
        throw new Error('WLOG 序号或毫秒/微秒相位错误');
      if (i) {
        const dt = (v[2] - rows[i-1][2]) >>> 0;
        if (dt < 9900 || dt > 15000) throw new Error('WLOG 采样间隔超出 9900..15000 us');
      }
    });
    return rows;
  }
  async function read(send, frames, progress = () => {}) {
    const a = String(await send('ANGLESTATUS')).split(',');
    const d = String(await send('PS2STATUS')).split(',');
    if (a.length !== 14 || a[0] !== 'ATEL' || a[2] !== '0' || a[12] !== '0' ||
        d.length !== 12 || d[0] !== 'PS2' || d[2] !== '0') throw new Error('先 STOP 并锁定手柄再下载 WLOG');
    const status = String(await send('WLOG,STATUS'));
    const p = status.split(',');
    if (p.length !== 7 || p[0] !== 'WLOG' || p[1] !== 'STATUS' ||
        !p.slice(2).every(x => /^\d+$/.test(x))) throw new Error('WLOG 状态格式错误');
    const [active, total, capacity, overflow, sequence] = p.slice(2).map(Number);
    if (active || !total || capacity !== 1024 || total > capacity || overflow || sequence !== total)
      throw new Error('WLOG 未停止、为空、溢出或序号不完整');
    const found = new Map();
    frames.length = 0;
    let cursor = 0;
    for (let offset = 0; offset < total; offset += 8) {
      const count = Math.min(8, total - offset);
      let complete = false, lastError;
      for (let attempt = 0; attempt < 3 && !complete; attempt++) {
        let reply;
        let previous = -1;
        try { reply = await send(`WLOG,READ,${offset},${count}`); }
        catch (error) { lastError = error; }
        for (; cursor < frames.length; cursor++) {
          const values = row(frames[cursor]), n = values[0];
          if (n < offset || n >= offset + count || n >= total || n <= previous) throw new Error('WLOG 批次序号越界或乱序');
          previous = n;
          if (found.has(n) && String(found.get(n)) !== String(values)) throw new Error('WLOG 冲突重传');
          found.set(n, values);
        }
        if (reply && reply !== `WLOG,END,${offset},${count},${total}`) throw new Error('WLOG END 校验失败');
        complete = !!reply && Array.from({length: count}, (_, i) => offset+i).every(i => found.has(i));
      }
      if (!complete) throw new Error(`WLOG 缺样或命令超时：${lastError?.message || offset}`);
      progress(found.size, total);
    }
    if (String(await send('WLOG,STATUS')) !== status) throw new Error('WLOG 下载期间被改变');
    return validate(Array.from({length: total}, (_, i) => found.get(i)));
  }
  function csv(rows) {
    const header = ['sequence','tick_ms','tick_us','enabled','armed','angle_mode'];
    for (const wheel of 'ABCD') header.push(`${wheel}_target_rpm`, `${wheel}_filtered_rpm_x10`, `${wheel}_raw_rpm_x10`, `${wheel}_pwm_mpercent`);
    header.push('saturated_mask','tof_limited','battery_mv','tof_mm','tof_valid','encoder_ready');
    return [header, ...rows].map(x => x.join(',')).join('\r\n')+'\r\n';
  }
  const api = {row, validate, read, csv};
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.WheelLog = api;
})(typeof globalThis !== 'undefined' ? globalThis : this);
