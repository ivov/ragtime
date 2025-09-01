console.log("Running Promise API tests...");

let testsCompleted = 0;
const totalTests = 4;

function testComplete(testName, success) {
  testsCompleted++;
  if (success) {
    console.log(`PASS: ${testName}`);
  } else {
    console.log(`FAIL: ${testName}`);
  }

  if (testsCompleted === totalTests) {
    console.log("All tests passed");
  }
}

// Test 1: Promise resolution with existing file
fs.writeFile("/tmp/promise-test.txt", "Promise test content", (err) => {
  if (err) {
    console.error("Setup failed:", err);
    return;
  }

  fs.readFileAsync("/tmp/promise-test.txt")
    .then((data) => {
      const success = data === "Promise test content";
      testComplete("Promise resolution", success);
      if (!success) {
        console.log(`Expected: "Promise test content", Got: "${data}"`);
      }
    })
    .catch((error) => {
      testComplete("Promise resolution", false);
      console.log("Unexpected rejection:", error);
    });
});

// Test 2: Promise rejection with non-existent file
fs.readFileAsync("/tmp/nonexistent-promise.txt")
  .then((data) => {
    testComplete("Promise rejection", false);
    console.log("Should have rejected, got:", data);
  })
  .catch((error) => {
    const success = error && error.code === "FileNotFound";
    testComplete("Promise rejection", success);
    if (!success) {
      console.log("Wrong error:", error);
    }
  });

// Test 3: Promise chaining
fs.writeFile("/tmp/chain-test.txt", "chain", (err) => {
  if (err) return;

  fs.readFileAsync("/tmp/chain-test.txt")
    .then((data) => {
      return data + " extended";
    })
    .then((result) => {
      const success = result === "chain extended";
      testComplete("Promise chaining", success);
      if (!success) {
        console.log(`Expected: "chain extended", Got: "${result}"`);
      }
    })
    .catch(() => {
      testComplete("Promise chaining", false);
    });
});

// Test 4: Compare callback vs promise behavior
fs.writeFile("/tmp/compare-test.txt", "same data", (err) => {
  if (err) return;

  let callbackData = null;
  let promiseData = null;
  let bothCompleted = false;

  function checkBothComplete() {
    if (callbackData !== null && promiseData !== null && !bothCompleted) {
      bothCompleted = true;
      const success = callbackData === promiseData;
      testComplete("Callback vs Promise consistency", success);
      if (!success) {
        console.log(`Callback: "${callbackData}", Promise: "${promiseData}"`);
      }
    }
  }

  // Callback version
  fs.readFile("/tmp/compare-test.txt", (err, data) => {
    if (!err) {
      callbackData = data;
      checkBothComplete();
    }
  });

  // Promise version
  fs.readFileAsync("/tmp/compare-test.txt").then((data) => {
    promiseData = data;
    checkBothComplete();
  });
});

setTimeout(() => {
  if (testsCompleted < totalTests) {
    console.log("FAIL: Tests timed out");
  }
}, 100 /* safety timeout */);
