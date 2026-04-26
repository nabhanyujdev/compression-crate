# COMPRESSION CRATE

Compression Crate is a full-stack file compression and integrity project built around a native C++ Huffman engine, a Flask API, and a drag-and-drop browser interface. The project is designed to be both practical and demonstrative: it compresses and restores files, verifies integrity with SHA-256, and exposes the algorithmic pipeline in a way that is easy to explain in a portfolio, interview, or systems project discussion.

## Project Overview

Compression Crate takes a file, analyzes byte frequencies, builds a Huffman tree, and writes the encoded result into a custom `.huf` archive. That archive also stores the metadata needed to reconstruct the file later, including a SHA-256 checksum of the original payload. When a user uploads the archive for decompression, the native engine restores the original file and validates that the resulting bytes match the embedded checksum.

The project intentionally combines three layers:

- A native systems layer in C++ for Huffman coding, archive serialization, and checksum handling
- A Python backend in Flask that accepts uploads and orchestrates the native binary
- A handcrafted frontend that makes the workflow feel polished and presentable instead of purely utilitarian

## Screenshots:
<img width="1470" height="831" alt="Screenshot 2026-04-26 at 4 00 09 PM" src="https://github.com/user-attachments/assets/2b35555a-c57a-41aa-adbb-b000b5df26aa" />
<img width="1470" height="833" alt="Screenshot 2026-04-26 at 4 00 20 PM" src="https://github.com/user-attachments/assets/22b199a5-fccc-48f4-9fce-8678ec2b7516" />
<img width="1470" height="414" alt="Screenshot 2026-04-26 at 4 00 31 PM" src="https://github.com/user-attachments/assets/5e14485c-498a-49bd-a79e-dd54eccdfd52" />


## Core Features

- Compress files into a custom `.huf` archive using Huffman encoding
- Restore `.huf` archives back into their original file form
- Verify file integrity using an embedded SHA-256 checksum
- Display before/after file sizes, compression ratio, and dominant symbol frequencies
- Offer both a native CLI workflow and a browser-based upload interface
- Return structured JSON from the native binary so the Flask layer can consume results cleanly

## Architecture

The application is split into three cooperating layers.

### 1. Native Compression Engine

Responsibilities:

- Read raw file bytes from disk
- Count symbol frequencies across the input
- Build a Huffman tree from those frequencies
- Generate prefix codes and bit-pack the encoded payload
- Compute a SHA-256 digest of the original file
- Serialize everything into a custom archive format
- Restore original data during decompression
- Verify integrity by recomputing the checksum on restored bytes

The engine supports three modes:

- `compress`
- `decompress`
- `inspect`

Each mode returns structured JSON to stdout so the backend can display meaningful results without scraping plain text output.

### 2. Flask API Layer

Responsibilities:

- Serve the frontend
- Accept multipart file uploads
- Persist uploaded files into a temporary uploads directory
- Generate output filenames for compressed and restored files
- Invoke the compiled C++ binary through `subprocess.run`
- Return JSON responses containing output metadata, file stats, and download links
- Serve processed files back to the user through download routes

This layer keeps the native implementation isolated while still making the project easy to demo in a browser.

### 3. Frontend Experience

Responsibilities:

- Provide drag-and-drop upload UX
- Let the user switch between compression and decompression flows
- Submit files asynchronously to the Flask API
- Render result cards with size metrics, checksum details, and dominant symbols
- Surface errors clearly when uploads or archive operations fail

The visual direction is intentionally editorial and tactile rather than a default app-dashboard look.

## Request Flow

1. The user chooses `Compress` or `Decompress` in the browser.
2. The frontend submits the selected file to `/api/process`.
3. Flask stores the upload and calls the native `bin/compressor` binary.
4. The C++ engine processes the file and prints a JSON summary.
5. Flask returns a response with computed stats and a download URL.
6. The frontend renders the result board and lets the user download the output file.

## Build Requirements and Dependencies

### System Requirements

- Python 3.9 or newer
- A C++17-compatible compiler
  - `clang++` works on macOS
  - `g++` or `clang++` works on Linux
- `make`

### Python Dependencies
Current direct dependency:

- `Flask`

The rest of the Python backend uses the standard library:

- `json`
- `subprocess`
- `uuid`
- `pathlib`

### Frontend Dependencies

None. The frontend uses plain HTML, CSS, and JavaScript.

### Native Dependencies

None beyond the C++ standard library. The SHA-256 implementation is included directly in the C++ source rather than pulled in from an external package.


## Local Setup

Install Python dependencies:

```bash
pip install -r requirements.txt
```

Build the native binary:

```bash
make
```

Run the app:

```bash
python3 app.py
```

Open:

```text
http://127.0.0.1:5000
```

## CLI Usage

Compress a file:

```bash
./bin/compressor compress path/to/input.txt path/to/output.huf
```

Decompress an archive:

```bash
./bin/compressor decompress path/to/archive.huf path/to/restored.txt
```

Inspect an archive:

```bash
./bin/compressor inspect path/to/archive.huf
```

Each command prints structured JSON to stdout.

## Project Structure

```text
.
├── app.py
├── Makefile
├── README.md
├── requirements.txt
├── src/
│   └── compressor.cpp
├── static/
│   ├── css/
│   │   └── styles.css
│   └── js/
│       └── app.js
├── templates/
│   └── index.html
```

## Notes and Tradeoffs

- Empty files are supported.
- Some files compress poorly, especially small inputs or already dense binary formats.
- For small files, archive metadata can outweigh compression gains.
- The default upload limit is 64 MB.

## Future Improvement Scope

- Add batch compression and decompression for multiple files
- Add archive previews and richer inspect-mode metadata in the web UI
- Add benchmark pages comparing compression behavior across file types
- Add background job handling for larger file uploads

## USP

It's is more than a CRUD-style upload demo. It combines:

- data structures and algorithms
- low-level binary serialization
- integrity verification
- file-processing UX
