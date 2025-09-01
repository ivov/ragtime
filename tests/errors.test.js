process.on("uncaughtException", (error) => {
  console.error("Caught error:", error.code);
  process.exit(1);
});

fs.readFile("nonexistent.txt", (err, _data) => {
  if (err.code === "FileNotFound") {
    console.log("PASS: Error code is FileNotFound");
    console.log("All tests passed");
  } else {
    console.error("FAIL: Expected 'FileNotFound', got:", err.code);
  }
});
