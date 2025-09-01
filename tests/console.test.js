// test: `console.log` method
console.log("LOG TEST: Basic log message");

// test: `console.warn` method
console.warn("LOG TEST: Warning message");

// test: `console.info` method
console.info("LOG TEST: Info message");

// test: `console.debug` method
console.debug("LOG TEST: Debug message");

// test: `console.error` method
console.error("LOG TEST: Error message");

// test: Invalid console calls
try {
  console.log(); // No arguments
  console.log("LOG TEST: Empty console.log handled");
} catch (e) {
  console.log("LOG TEST: Console error caught");
}

console.log("All tests passed");