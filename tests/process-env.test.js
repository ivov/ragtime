console.log("Starting process.env test");

// Test 1: Check that process.env exists
if (!process.env) {
  console.error("FAIL: process.env is undefined");
  process.exit(1);
}

// Test 2: Check that process.env is an object
if (typeof process.env !== "object") {
  console.error("FAIL: process.env is not an object");
  process.exit(1);
}

// Test 3: Check that we can read environment variables
const keys = Object.keys(process.env);
if (keys.length === 0) {
  console.error("FAIL: No environment variables found");
  process.exit(1);
}

// Test 4: Check that PATH exists (should exist on all systems)
if (!process.env.PATH) {
  console.error("FAIL: PATH environment variable not found");
  process.exit(1);
}

console.log("All tests passed");
console.log("Found", keys.length, "environment variables");

