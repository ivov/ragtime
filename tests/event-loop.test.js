// test: Multiple async operations coordination
console.log("EVENTLOOP TEST: Starting multiple async operations");

let completedOperations = 0;
const expectedOperations = 3;

// test: `setTimeout` in event loop
setTimeout(() => {
    console.log("EVENTLOOP TEST: setTimeout completed");
    completedOperations++;
    checkCompletion();
}, 100);

// test: `setInterval` with automatic clearing
let intervalCount = 0;
const intervalId = setInterval(() => {
    intervalCount++;
    console.log(`EVENTLOOP TEST: setInterval ${intervalCount}`);
    if (intervalCount >= 2) {
        clearInterval(intervalId);
        completedOperations++;
        checkCompletion();
    }
}, 50);

// test: Nested `setTimeout` operations
setTimeout(() => {
    setTimeout(() => {
        console.log("EVENTLOOP TEST: nested setTimeout completed");
        completedOperations++;
        checkCompletion();
    }, 50);
}, 25);

function checkCompletion() {
    if (completedOperations === expectedOperations) {
        console.log("All tests passed");
    }
}
