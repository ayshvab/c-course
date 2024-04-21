#!/bin/bash

TARGET=./convert_to_utf8.exe

# Function to perform tests and compare results
function test_conversion() {
  encoding="$1"
  input_file="assets/$1.txt"
  output_file="utf8_from_$1.txt"
  expected_file="expected_utf8_from_$1.txt"

  # Run your program to convert
  ./$TARGET "$encoding" "$input_file" "$output_file"

  # Generate expected output using iconv
  iconv -f "$encoding" -t utf8 "$input_file" > "$expected_file"

  # Compare generated and expected output
  diff "$output_file" "$expected_file"
  if [ $? -eq 0 ]; then
    echo "✅  Test passed for $encoding encoding."
  else
    echo "❌  Test failed for $encoding encoding. Diff output above."
  fi

  # Clean up temporary expected output file
  rm "$expected_file"
}

# Test cp1251 conversion
test_conversion cp1251

# Test koi8 conversion (assuming koi8 encoding)
test_conversion koi8

test_conversion iso-8859-5

# Add more test_conversion calls for other encodings you want to test

echo "All tests completed."

