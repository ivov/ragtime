// test: Basic module loading
console.log("Starting require test");

const math = require("./math.js");

if (math.add(2, 3) !== 5) {
  console.error("FAIL: Module function returned wrong value");
  process.exit(1);
}

if (math.PI !== 3.14159) {
  console.error("FAIL: Module property incorrect");
  process.exit(1);
}

// test: Module caching
const math2 = require("./math.js");
if (math2.add(5, 7) !== 12) {
  console.error("FAIL: Cached module broken");
  process.exit(1);
}

console.log("All tests passed");
