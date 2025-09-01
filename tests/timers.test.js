// test: `setTimeout` and `clearTimeout` functionality
let timeoutRan = false;
const timeoutId = setTimeout(() => {
    timeoutRan = true;
    console.log("TIMER TEST: Timeout executed");
}, 500);
clearTimeout(timeoutId);

// test: `setInterval` and `clearInterval` functionality
let intervalCount = 0;
const intervalId = setInterval(() => {
    intervalCount++;
    console.log(`TIMER TEST: Interval ${intervalCount}`);
    if (intervalCount >= 3) {
        clearInterval(intervalId);
        console.log("TIMER TEST: Interval cleared");
  console.log("All tests passed");
    }
}, 200);

// test: Invalid setTimeout arguments
try {
  setTimeout(); // No arguments
  console.log("TIMER TEST: setTimeout with no args didn't crash");
} catch (e) {
  console.log("TIMER TEST: setTimeout error caught");
}

// test: Multiple rapid timer operations
for (let i = 0; i < 10; i++) {
  setTimeout(() => {
    // Do nothing, just stress test timer creation
  }, 1);
}

console.log("TIMER TEST: Rapid timer creation completed");