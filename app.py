import json
import subprocess
import uuid
from pathlib import Path

from flask import Flask, jsonify, render_template, request, send_from_directory
from werkzeug.utils import secure_filename


BASE_DIR = Path(__file__).resolve().parent
UPLOAD_DIR = BASE_DIR / "uploads"
OUTPUT_DIR = BASE_DIR / "output"
BIN_DIR = BASE_DIR / "bin"
COMPRESSOR_BIN = BIN_DIR / "compressor"
MAX_CONTENT_LENGTH = 64 * 1024 * 1024

ALLOWED_ACTIONS = {"compress", "decompress"}

app = Flask(__name__)
app.config["MAX_CONTENT_LENGTH"] = MAX_CONTENT_LENGTH


def ensure_runtime_dirs() -> None:
    for directory in (UPLOAD_DIR, OUTPUT_DIR, BIN_DIR):
        directory.mkdir(parents=True, exist_ok=True)


def build_download_url(kind: str, filename: str) -> str:
    return f"/download/{kind}/{filename}"


def run_compressor(action: str, input_path: Path, output_path: Path) -> dict:
    completed = subprocess.run(
        [str(COMPRESSOR_BIN), action, str(input_path), str(output_path)],
        capture_output=True,
        text=True,
        cwd=BASE_DIR,
        check=False,
    )

    stdout = completed.stdout.strip()
    stderr = completed.stderr.strip()

    try:
        payload = json.loads(stdout) if stdout else {}
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"Compressor returned unreadable output: {exc}") from exc

    if completed.returncode != 0 or not payload.get("ok"):
        message = payload.get("error") or stderr or "Compression engine failed."
        raise RuntimeError(message)

    return payload


def prepared_name(filename: str, action: str) -> str:
    safe_name = secure_filename(filename) or "upload.bin"
    stem = Path(safe_name).stem or "payload"
    suffix = Path(safe_name).suffix
    token = uuid.uuid4().hex[:8]
    if action == "compress":
        return f"{stem}-{token}{suffix}.huf"
    if suffix == ".huf":
        restored_stem = Path(stem).stem or stem
        restored_suffix = Path(stem).suffix or ".bin"
        return f"{restored_stem}-restored-{token}{restored_suffix}"
    return f"{stem}-restored-{token}{suffix or '.bin'}"


@app.get("/")
def index():
    ensure_runtime_dirs()
    return render_template("index.html")


@app.get("/api/health")
def health():
    return jsonify(
        {
            "ok": True,
            "compressor_ready": COMPRESSOR_BIN.exists(),
            "max_upload_mb": MAX_CONTENT_LENGTH // (1024 * 1024),
        }
    )


@app.post("/api/process")
def process_file():
    ensure_runtime_dirs()

    action = request.form.get("action", "").strip().lower()
    if action not in ALLOWED_ACTIONS:
        return jsonify({"ok": False, "error": "Unsupported action."}), 400

    if "file" not in request.files:
        return jsonify({"ok": False, "error": "No file uploaded."}), 400

    if not COMPRESSOR_BIN.exists():
        return jsonify({"ok": False, "error": "Native compressor binary not found. Build the project first."}), 500

    uploaded = request.files["file"]
    if not uploaded.filename:
        return jsonify({"ok": False, "error": "Choose a file to continue."}), 400

    source_name = secure_filename(uploaded.filename) or "upload.bin"
    upload_token = uuid.uuid4().hex[:10]
    input_path = UPLOAD_DIR / f"{upload_token}-{source_name}"
    uploaded.save(input_path)

    output_name = prepared_name(source_name, action)
    output_path = OUTPUT_DIR / output_name

    try:
        payload = run_compressor(action, input_path, output_path)
    except Exception as exc:
        input_path.unlink(missing_ok=True)
        output_path.unlink(missing_ok=True)
        return jsonify({"ok": False, "error": str(exc)}), 500

    response = {
        "ok": True,
        "action": action,
        "source_name": source_name,
        "output_name": output_name,
        "download_url": build_download_url("output", output_name),
        "input_download_url": build_download_url("uploads", input_path.name),
        "stats": payload,
    }
    return jsonify(response)


@app.get("/download/<kind>/<path:filename>")
def download(kind: str, filename: str):
    if kind == "output":
        directory = OUTPUT_DIR
    elif kind == "uploads":
        directory = UPLOAD_DIR
    else:
        return jsonify({"ok": False, "error": "Unknown download bucket."}), 404
    return send_from_directory(directory, filename, as_attachment=True)


if __name__ == "__main__":
    ensure_runtime_dirs()
    app.run(debug=True)
