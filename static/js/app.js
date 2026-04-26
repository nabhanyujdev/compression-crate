const form = document.getElementById("upload-form");
const fileInput = document.getElementById("file-input");
const actionInput = document.getElementById("action-input");
const submitBtn = document.getElementById("submit-btn");
const fileLabel = document.getElementById("file-label");
const results = document.getElementById("results");
const statusPill = document.getElementById("status-pill");
const dropzone = document.getElementById("dropzone");
const dropzoneTitle = document.getElementById("dropzone-title");
const dropzoneSubtitle = document.getElementById("dropzone-subtitle");

const modeConfig = {
  compress: {
    title: "Prepare a file for Huffman compression",
    subtitle: "Best for text-heavy or repetitive files, though any binary file will run through the engine.",
    button: "Run compression",
    status: "Ready to compress",
  },
  decompress: {
    title: "Restore a .huf archive and verify integrity",
    subtitle: "Upload an archive created by this tool to restore the original file and re-check its checksum.",
    button: "Run decompression",
    status: "Ready to decompress",
  },
};

function formatBytes(value) {
  if (value === 0) return "0 B";
  const units = ["B", "KB", "MB", "GB"];
  const exponent = Math.min(Math.floor(Math.log(value) / Math.log(1024)), units.length - 1);
  return `${(value / 1024 ** exponent).toFixed(exponent === 0 ? 0 : 2)} ${units[exponent]}`;
}

function setStatus(message, tone = "") {
  statusPill.textContent = message;
  statusPill.className = `status-pill ${tone}`.trim();
}

function updateFileLabel() {
  const file = fileInput.files?.[0];
  fileLabel.textContent = file ? `${file.name} · ${formatBytes(file.size)}` : "No file selected yet";
}

function switchMode(mode) {
  actionInput.value = mode;
  const config = modeConfig[mode];
  dropzoneTitle.textContent = config.title;
  dropzoneSubtitle.textContent = config.subtitle;
  submitBtn.textContent = config.button;
  setStatus(config.status);

  document.querySelectorAll(".mode-btn").forEach((button) => {
    button.classList.toggle("active", button.dataset.mode === mode);
  });
}

function renderSymbols(symbols = []) {
  if (!symbols.length) {
    return `<p class="result-meta">No dominant symbol data available for this file.</p>`;
  }
  return `
    <div class="symbol-list">
      ${symbols
        .map((entry) => `<span class="symbol-pill">${entry.symbol} · ${entry.count}</span>`)
        .join("")}
    </div>
  `;
}

function renderResultCard(payload) {
  const stats = payload.stats;
  const isCompress = payload.action === "compress";
  const title = isCompress ? "Compression finished" : "Decompression finished";
  const integrity = stats.integrity_ok === false ? "Failed" : "Verified";
  const archiveOutcome =
    isCompress && Number(stats.space_saved_percent) < 0
      ? "This file expanded in archive form, which is normal for some smaller or already dense inputs."
      : isCompress
        ? "This file benefited from Huffman coding and now has a smaller archive footprint."
        : "The archive restored successfully and the checksum matched the original source file.";
  const primaryMetrics = isCompress
    ? [
        ["Original size", formatBytes(stats.original_size)],
        ["Archive size", formatBytes(stats.output_size)],
        ["Archive / original", `${stats.compression_ratio}x`],
        ["Space saved", `${stats.space_saved_percent}%`],
      ]
    : [
        ["Archive size", formatBytes(stats.archive_size)],
        ["Restored size", formatBytes(stats.restored_size)],
        ["Expansion ratio", `${stats.expansion_ratio}x`],
        ["Integrity", integrity],
      ];

  return `
    <article class="result-card">
      <div class="result-top">
        <div>
          <h3>${title}</h3>
          <p class="result-meta">
            Source: ${payload.source_name}<br />
            Output: ${payload.output_name}<br />
            SHA-256: ${stats.checksum}
          </p>
          <p class="result-meta">${archiveOutcome}</p>
        </div>
        <div class="browse-chip">${payload.action.toUpperCase()}</div>
      </div>
      <div class="metric-grid">
        ${primaryMetrics
          .map(
            ([label, value]) => `
              <div class="metric">
                <div class="metric-label">${label}</div>
                <div class="metric-value">${value}</div>
              </div>
            `,
          )
          .join("")}
      </div>
      <h4>Dominant symbols</h4>
      ${renderSymbols(stats.top_symbols)}
      <a class="download-link" href="${payload.download_url}">Download result</a>
    </article>
  `;
}

document.querySelectorAll(".mode-btn").forEach((button) => {
  button.addEventListener("click", () => switchMode(button.dataset.mode));
});

["dragenter", "dragover"].forEach((eventName) => {
  dropzone.addEventListener(eventName, (event) => {
    event.preventDefault();
    dropzone.classList.add("dragover");
  });
});

["dragleave", "drop"].forEach((eventName) => {
  dropzone.addEventListener(eventName, (event) => {
    event.preventDefault();
    dropzone.classList.remove("dragover");
  });
});

dropzone.addEventListener("drop", (event) => {
  const files = event.dataTransfer?.files;
  if (!files?.length) return;
  const transfer = new DataTransfer();
  Array.from(files).forEach((file) => transfer.items.add(file));
  fileInput.files = transfer.files;
  updateFileLabel();
});

fileInput.addEventListener("change", updateFileLabel);

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  if (!fileInput.files?.length) {
    setStatus("Select a file first", "error");
    return;
  }

  submitBtn.disabled = true;
  setStatus("Processing file...");
  results.innerHTML = `<div class="results-placeholder">Running the native engine and collecting telemetry...</div>`;

  const data = new FormData(form);

  try {
    const response = await fetch("/api/process", { method: "POST", body: data });
    const payload = await response.json();

    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || "Something went wrong.");
    }

    results.innerHTML = renderResultCard(payload);
    setStatus("Operation completed", "success");
  } catch (error) {
    results.innerHTML = `<div class="results-placeholder">${error.message}</div>`;
    setStatus("Operation failed", "error");
  } finally {
    submitBtn.disabled = false;
  }
});

switchMode("compress");
