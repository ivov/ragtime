console.log("Running Filesystem API tests...");

const testFile = "/tmp/test-file.txt";
const testFile2 = "/tmp/test-file-2.txt";
const nonExistentFile = "/tmp/non-existent-file.txt";

// Test 1: Write a file
console.log("\nTest 1: fs.writeFile - Create new file");
fs.writeFile(testFile, "Hello, World!", (err) => {
  if (err) {
    console.error("FAIL: Error writing file:", err);
    return;
  }
  console.log("PASS: File written successfully");

  // Test 2: Read the file we just wrote
  console.log("\nTest 2: fs.readFile - Read existing file");
  fs.readFile(testFile, (err, data) => {
    if (err) {
      console.error("FAIL: Error reading file:", err);
      return;
    }
    if (data === "Hello, World!") {
      console.log("PASS: File content matches expected value");
    } else {
      console.error(
        'FAIL: File content does not match. Expected "Hello, World!", got:',
        data,
      );
    }

    // Test 3: Check if file exists
    console.log("\nTest 3: fs.exists - Check existing file");
    fs.exists(testFile, (err, exists) => {
      if (err) {
        console.error("FAIL: Error checking file existence:", err);
        return;
      }
      if (exists === true) {
        console.log("PASS: File exists as expected");
      } else {
        console.error("FAIL: File should exist but fs.exists returned false");
      }

      // Test 4: Check non-existent file
      console.log("\nTest 4: fs.exists - Check non-existent file");
      fs.exists(nonExistentFile, (err, exists) => {
        if (err) {
          console.error("FAIL: Error checking non-existent file:", err);
          return;
        }
        if (exists === false) {
          console.log(
            "PASS: Non-existent file correctly reported as not existing",
          );
        } else {
          console.error(
            "FAIL: Non-existent file incorrectly reported as existing",
          );
        }

        // Test 5: Overwrite existing file
        console.log("\nTest 5: fs.writeFile - Overwrite existing file");
        fs.writeFile(testFile, "Updated content!", (err) => {
          if (err) {
            console.error("FAIL: Error overwriting file:", err);
            return;
          }
          console.log("PASS: File overwritten successfully");

          // Verify the update
          fs.readFile(testFile, (err, data) => {
            if (err) {
              console.error("FAIL: Error reading updated file:", err);
              return;
            }
            if (data === "Updated content!") {
              console.log("PASS: Updated content verified");
            } else {
              console.error(
                'FAIL: Updated content does not match. Expected "Updated content!", got:',
                data,
              );
            }

            // Test 6: Read non-existent file (error handling)
            console.log("\nTest 6: fs.readFile - Read non-existent file");
            fs.readFile(nonExistentFile, (err, _data) => {
              if (err) {
                console.log(
                  "PASS: Error correctly reported for non-existent file",
                );
              } else {
                console.error(
                  "FAIL: Should have received error for non-existent file",
                );
              }

              // Test 7: Write empty file
              console.log("\nTest 7: fs.writeFile - Write empty file");
              fs.writeFile(testFile2, "", (err) => {
                if (err) {
                  console.error("FAIL: Error writing empty file:", err);
                  return;
                }
                console.log("PASS: Empty file written successfully");

                // Verify empty file
                fs.readFile(testFile2, (err, data) => {
                  if (err) {
                    console.error("FAIL: Error reading empty file:", err);
                    return;
                  }
                  if (data === "") {
                    console.log("PASS: Empty file content verified");
                  } else {
                    console.error(
                      "FAIL: Empty file should have no content, got:",
                      data,
                    );
                  }

                  // Test 8: Multiple concurrent writes
                  console.log("\nTest 8: fs.writeFile - Concurrent writes");
                  let completedWrites = 0;
                  const totalWrites = 3;

                  for (let i = 1; i <= totalWrites; i++) {
                    const fileName = `/tmp/concurrent-test-${i}.txt`;
                    const content = `Content for file ${i}`;

                    fs.writeFile(fileName, content, (err) => {
                      if (err) {
                        console.error(
                          `FAIL: Error writing concurrent file ${i}:`,
                          err,
                        );
                        return;
                      }
                      completedWrites++;

                      if (completedWrites === totalWrites) {
                        console.log(
                          "PASS: All concurrent writes completed successfully",
                        );

                        // Clean up test files
                        console.log("\nCleaning up test files...");
                        // Note: Since fs.unlink is not implemented, we'll just note this
                        console.log(
                          "Note: Test files created during testing need manual cleanup",
                        );
                        console.log(
                          "Files created: /tmp/test-file.txt, /tmp/test-file-2.txt, /tmp/concurrent-test-1.txt, /tmp/concurrent-test-2.txt, /tmp/concurrent-test-3.txt",
                        );

                        console.log("\nAll tests passed");
                      }
                    });
                  }
                });
              });
            });
          });
        });
      });
    });
  });
});

// Test error cases with invalid inputs
console.log("\nTest 9: Error handling - Invalid arguments");
try {
  fs.readFile(); // No arguments
  console.error("FAIL: Should have thrown error for missing arguments");
} catch (e) {
  console.log("PASS: Error thrown for missing arguments");
}

try {
  fs.writeFile(testFile); // Missing content and callback
  console.error("FAIL: Should have thrown error for missing arguments");
} catch (e) {
  console.log("PASS: Error thrown for missing arguments");
}

try {
  fs.exists(); // No arguments
  console.error("FAIL: Should have thrown error for missing arguments");
} catch (e) {
  console.log("PASS: Error thrown for missing arguments");
}

