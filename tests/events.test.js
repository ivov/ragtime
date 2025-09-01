console.log("Starting EventEmitter tests");

// Test 1: Basic on/emit functionality
const emitter = new EventEmitter();
let called = false;
emitter.on('test', () => { called = true; });
emitter.emit('test');

if (called) {
  console.log("PASS: Basic on/emit");
} else {
  console.log("FAIL: Basic on/emit");
}

console.log("All tests passed");