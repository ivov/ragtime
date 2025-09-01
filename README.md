# ragtime

Toy JavaScript runtime written in C with [JSC](https://developer.apple.com/documentation/javascriptcore) and [libuv](https://libuv.org/).

- Timer API
  - `setTimeout`
  - `setInterval`
  - `clearTimeout`
  - `clearInterval`
- Filesystem API
  - `fs.readFile` (callback-based)
  - `fs.readFileAsync` (promise-based)
  - `fs.writeFile`
  - `fs.exists`
- Streams API
  - `fs.createReadStream`
  - `fs.createWriteStream`
- HTTP API
  - `http.createServer`
  - `http.get`
  - `net.createServer`
- Process API
  - `process.argv`
  - `process.env`
  - `process.exit`
  - `process.on`
- Console API
  - `console.log` (plus variants)
- Module API
  - `require` (with caching)
- Events API
  - `EventEmitter`

## Usage

```bash
make setup/mac # or make setup/linux
make build
make test
make help

./build/ragtime --help
./build/ragtime path/to/script.js
./build/ragtime --eval "console.log('Hello, world!');"
```

## Examples

Timer API:

```javascript
setTimeout(() => {
  console.log("Timeout fired after 1 second!");
}, 1000);

let count = 0;
const interval = setInterval(() => {
  count++;
  console.log(`Interval tick ${count}`);

  if (count >= 3) {
    clearInterval(interval);
    console.log("Interval cleared - complete!");
  }
}, 500);
```

Filesystem API:

```javascript
fs.writeFile("test.txt", "Hello!", (err) => {
  if (err) {
    console.error("Write error:", err);
    return;
  }

  console.log("File written successfully");

  fs.readFile("test.txt", (err, data) => {
    if (err) {
      console.error("Error:", err);
      return;
    }

    console.log("Content:", data);

    fs.exists("test.txt", (err, exists) => {
      console.log("File exists: ", exists);
    });
  });
});

fs.readFileAsync("test.txt")
  .then((data) => {
    console.log("Content: ", data);
    return "processed: " + data;
  })
  .catch((error) => {
    console.error("Error: ", error.code, error.message);
  });
```

Streams API:

```javascript
const readStream = fs.createReadStream("input.txt");
const writeStream = fs.createWriteStream("output.txt");

readStream.on("data", (chunk) => {
  console.log("Received chunk:", chunk);
});

readStream.on("end", () => {
  console.log("Stream ended");
});

readStream.pipe(writeStream);

readStream.pause();
setTimeout(() => {
  console.log("Resuming stream...");
  readStream.resume();
}, 1000);
```

HTTP API:

```javascript
console.log("HTTP server starting...");

const server = http.createServer((req, res) => {
  console.log(`Request: ${req.method} ${req.url}`);
  res.end();
});

server.listen(8080);
console.log("Server listening on port 8080");
console.log("Test with: curl http://localhost:8080/test");

setInterval(() => {
  console.log("Server heartbeat...");
}, 5000);
```

Process API:

```javascript
console.log("Command line arguments:", process.argv);

process.on("uncaughtException", (error) => {
  console.error("Caught uncaught exception:", error.message);
  console.log("Exiting gracefully...");
  process.exit(1);
});

setTimeout(() => {
  console.log("Work completed successfully!");

  // Check if user passed "error" argument
  if (process.argv.includes("error")) {
    console.log("Throwing error as requested...");
    throw new Error("Demo error triggered by user");
  }

  console.log("Exiting normally...");
  process.exit(0);
}, 1000);

console.log("Waiting for work to complete...");
```

Module API:

```javascript
// math.js
module.exports = {
  add: function (a, b) {
    return a + b;
  },
  multiply: function (a, b) {
    return a * b;
  },
  PI: 3.14159,
};

// main.js
const math = require("./math.js");

console.log("2 + 3 =", math.add(2, 3));
console.log("4 * 5 =", math.multiply(4, 5));
console.log("PI =", math.PI);

const math2 = require("./math.js");
console.log("Cached module works:", math2.add(10, 20));

console.log("Module system demo complete!");
```

Events API:

```javascript
const emitter = new EventEmitter();

emitter.on("data", (value) => {
  console.log("Received data:", value);
});

emitter.on("error", (err) => {
  console.error("Error occurred:", err);
});

emitter.emit("data", { id: 1, message: "Hello" });
emitter.emit("data", { id: 2, message: "World" });
emitter.emit("error", "Oh no!");
```
