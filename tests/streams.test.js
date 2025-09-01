console.log("Starting stream tests");

// First create a test file
fs.writeFile("/tmp/stream-test-input.txt", "Hello, streams!\nThis is line 2.\nAnd here's line 3.", function(err) {
  if (err) {
    console.error("Failed to create test file:", err);
    return;
  }
  
  console.log("Test file created");
  
  // Test 1: Basic readable stream
  console.log("\nTest 1: Basic readable stream");
  var readStream = fs.createReadStream("/tmp/stream-test-input.txt");
  var chunks = [];
  
  readStream.on("data", function(chunk) {
    console.log("Received chunk:", chunk);
    chunks.push(chunk);
  });
  
  readStream.on("end", function() {
    console.log("PASS: Readable stream ended");
    console.log("Total chunks received:", chunks.length);
    
    // Test 2: Basic writable stream
    console.log("\nTest 2: Basic writable stream");
    var writeStream = fs.createWriteStream("/tmp/stream-test-output.txt");
    
    var canWrite = writeStream.write("First line\n");
    console.log("First write returned:", canWrite);
    
    canWrite = writeStream.write("Second line\n");
    console.log("Second write returned:", canWrite);
    
    writeStream.end();
    
    writeStream.on("finish", function() {
      console.log("PASS: Writable stream finished");
      
      // Test 3: Pipe functionality
      console.log("\nTest 3: Pipe functionality");
      var readStream2 = fs.createReadStream("/tmp/stream-test-input.txt");
      var writeStream2 = fs.createWriteStream("/tmp/stream-test-piped.txt");
      
      readStream2.pipe(writeStream2);
      
      writeStream2.on("finish", function() {
        console.log("PASS: Pipe completed");
        
        // Test 4: Pause and resume
        console.log("\nTest 4: Pause and resume");
        var readStream3 = fs.createReadStream("/tmp/stream-test-input.txt");
        var dataCount = 0;
        
        readStream3.on("data", function(chunk) {
          dataCount++;
          console.log("Data event", dataCount, "- chunk length:", chunk.length);
          
          if (dataCount === 1) {
            console.log("Pausing stream...");
            readStream3.pause();
            
            setTimeout(function() {
              console.log("Resuming stream...");
              readStream3.resume();
            }, 100);
          }
        });
        
        readStream3.on("end", function() {
          console.log("PASS: Pause/resume test completed");
          
          // Test 5: Simple write test
          console.log("\nTest 5: Simple write test");
          var writeSimple = fs.createWriteStream("/tmp/stream-test-simple.txt");
          
          writeSimple.write("Line 1\n");
          writeSimple.write("Line 2\n");
          writeSimple.write("Line 3\n");
          writeSimple.end();
          
          writeSimple.on("finish", function() {
            console.log("PASS: Simple write completed");
            console.log("\nAll tests passed");
          });
        });
      });
    });
  });
  
  readStream.on("error", function(err) {
    console.error("Read stream error:", err);
  });
});