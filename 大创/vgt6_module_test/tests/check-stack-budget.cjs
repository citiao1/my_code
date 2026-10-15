const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

const root = path.resolve(__dirname, '..');
const ioc = fs.readFileSync(path.join(root, 'vgt6_module_test.ioc'), 'utf8');
const startup = fs.readFileSync(path.join(root, 'MDK-ARM/startup_stm32f407xx.s'), 'utf8');
const map = fs.readFileSync(path.join(root, 'MDK-ARM/vgt6_module_test/vgt6_module_test.map'), 'utf8');
const graph = fs.readFileSync(path.join(root, 'MDK-ARM/vgt6_module_test/vgt6_module_test.htm'), 'utf8');
const monitor = fs.readFileSync(path.join(root, 'Bsp/Inc/stack_usage.h'), 'utf8');
const configured = Number(ioc.match(/^ProjectManager.StackSize=(\S+)/m)?.[1]);
const assembly = Number(startup.match(/Stack_Size\s+EQU\s+(0x[0-9a-f]+)/i)?.[1]);
const linked = Number(map.match(/Stack_Mem\s+0x[0-9a-f]+\s+Data\s+(\d+)/i)?.[1]);
const knownDepth = Number(graph.match(/Maximum Stack Usage\s*=\s*(\d+)\s+bytes/)?.[1]);
const monitored = Number(monitor.match(/STACK_USAGE_BUDGET_BYTES\s+(\d+)U/)?.[1]);

assert.ok([configured, assembly, linked, knownDepth].every(Number.isFinite), 'missing stack evidence');
assert.equal(assembly, configured, 'CubeMX configuration and startup stack differ');
assert.equal(linked, configured, 'linked image has a stale/incorrect stack reservation');
assert.equal(monitored, configured, 'runtime stack monitor and allocated area differ');
assert.ok(linked >= 4096, `stack ${linked} must reserve at least 4096 bytes`);
// The indirect command callback and asynchronous IRQ/FPU frames are not
// fully included in Keil's static graph. Reserve additional headroom.
assert.ok(linked >= knownDepth + 1024,
  `stack ${linked} is below known call depth ${knownDepth} plus 1024-byte caller/IRQ margin`);
console.log(`PASS: linked stack ${linked} bytes; known static depth ${knownDepth}; extra headroom ${linked - knownDepth}`);
console.log('Static graph has indirect/unknown paths; use the on-board STACK watermark for runtime evidence.');
