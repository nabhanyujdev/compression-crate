# Signal Press

Signal Press is a full-stack file compression and integrity project built around a native C++ Huffman engine, a Flask API, and a drag-and-drop browser interface. The project is designed to be both practical and demonstrative: it compresses and restores files, verifies integrity with SHA-256, and exposes the algorithmic pipeline in a way that is easy to explain in a portfolio, interview, or systems project discussion.

## Project Overview

Signal Press takes a file, analyzes byte frequencies, builds a Huffman tree, and writes the encoded result into a custom `.huf` archive. That archive also stores the metadata needed to reconstruct the file later, including a SHA-256 checksum of the original payload. When a user uploads the archive for decompression, the native engine restores the original file and validates that the resulting bytes match the embedded checksum.

The project intentionally combines three layers:

- A native systems layer in C++ for Huffman coding, archive serialization, and checksum handling
- A Python backend in Flask that accepts uploads and orchestrates the native binary
- A handcrafted frontend that makes the workflow feel polished and presentable instead of purely utilitarian

This makes the project useful as both an engineering artifact and a strong CV or portfolio piece, since it demonstrates data structures, file I/O, serialization, API design, and frontend UX in one build.

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

File: [src/compressor.cpp](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/src/compressor.cpp)

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

File: [app.py](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/app.py)

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

Files:

- [templates/index.html](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/templates/index.html)
- [static/css/styles.css](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/static/css/styles.css)
- [static/js/app.js](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/static/js/app.js)

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

Runtime Python dependencies are listed in [requirements.txt](/Users/nabhanyu.archive/Documents/Codex/2026-04-26/files-mentioned-by-the-user-screenshot-2/requirements.txt).

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
├── bin/
├── output/
├── src/
│   └── compressor.cpp
├── static/
│   ├── css/
│   │   └── styles.css
│   └── js/
│       └── app.js
├── templates/
│   └── index.html
└── uploads/
```

## Notes and Tradeoffs

- Empty files are supported.
- Some files compress poorly, especially small inputs or already dense binary formats.
- For small files, archive metadata can outweigh compression gains.
- The current archive format is designed for clarity and portability within this project, not for compatibility with standard archive tools.
- The Flask app currently uses the development server, which is fine for demos but not ideal for production deployment.
- The default upload limit is 64 MB.

## Future Improvement Scope

- Add batch compression and decompression for multiple files
- Add archive previews and richer inspect-mode metadata in the web UI
- Support folder compression or multi-file bundle archives
- Improve compression efficiency by reducing metadata overhead further
- Add benchmark pages comparing compression behavior across file types
- Add unit and integration tests for archive round-tripping and checksum validation
- Add Docker support for easier cross-platform setup
- Add production deployment support with Gunicorn or another WSGI server
- Add background job handling for larger file uploads
- Add optional archive encryption on top of integrity verification

## Why This Project Stands Out

Signal Press is more than a CRUD-style upload demo. It combines:

- data structures and algorithms
- low-level binary serialization
- integrity verification
- backend orchestration
- file-processing UX

That combination makes it a strong systems-focused portfolio project with a visible user interface and a meaningful technical core.
