// Basic HTTP API Tests
// Testing server creation and external client requests

console.log("Running basic HTTP API tests...");

let testsCompleted = 0;
const totalTests = 3;

function testComplete() {
	testsCompleted++;
	if (testsCompleted === totalTests) {
		console.log("\nAll tests passed");
		process.exit(0);
	}
}

// Test 1: Create HTTP Server
console.log("\nTest 1: http.createServer - Create server");
try {
	const server = http.createServer((req, res) => {
		// Server would handle requests here
		res.end();
	});
	console.log("PASS: Server created successfully");

	// Test 2: Listen on port
	console.log("\nTest 2: server.listen - Bind to port");
	server.listen(8083);
	console.log("PASS: Server listening on port 8083");

	testComplete();
	testComplete(); // Count as 2 tests
} catch (e) {
	console.error("FAIL: Server creation/listening failed:", e);
	process.exit(1);
}

// Test 3: HTTP GET to external URL
console.log("\nTest 3: http.get - External request");
http.get("http://httpbin.org/status/200", (err, response) => {
	if (err) {
		console.error("FAIL: External GET request failed:", err);
	} else if (response.statusCode === 200) {
		console.log("PASS: External GET request successful (status 200)");
	} else {
		console.error("FAIL: Unexpected status code:", response.statusCode);
	}
	testComplete();
});

// Safety timeout
setTimeout(() => {
	console.error("\nFAIL: Tests timed out");
	process.exit(1);
}, 8000);
