#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// Include the complete HTML content here (from previous response)
// For brevity, I'm showing the structure - you'd paste the full HTML here

const char MAIN_PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>EEPROM Programmer Wizard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .step { display: none; padding: 20px; border: 1px solid #ddd; border-radius: 5px; margin-bottom: 20px; }
        .step.active { display: block; }
        .btn { background: #007cba; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        .btn:hover { background: #005a87; }
        .btn:disabled { background: #ccc; cursor: not-allowed; }
        .btn-danger { background: #dc3545; }
        .progress { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; margin: 10px 0; }
        .progress-bar { height: 100%; background: #007cba; border-radius: 10px; width: 0%; transition: width 0.3s; }
        .log { background: #000; color: #0f0; padding: 10px; border-radius: 5px; height: 200px; overflow-y: scroll; font-family: monospace; font-size: 12px; }
        .status { padding: 10px; border-radius: 5px; margin: 10px 0; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        .warning { background: #fff3cd; color: #856404; border: 1px solid #ffeaa7; }
        .step-indicator { display: flex; justify-content: center; margin: 20px 0; }
        .step-dot { width: 30px; height: 30px; border-radius: 50%; background: #e0e0e0; display: flex; align-items: center; justify-content: center; margin: 0 10px; font-weight: bold; }
        .step-dot.active { background: #007cba; color: white; }
        .file-input { margin: 15px 0; padding: 15px; border: 2px dashed #ccc; border-radius: 5px; text-align: center; }
        .connection-info { background: #e3f2fd; padding: 15px; border-radius: 5px; margin-bottom: 20px; border-left: 4px solid #2196f3; }
        .memory-warning { background: #fff3cd; padding: 10px; border-radius: 5px; margin: 10px 0; font-size: 12px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>EEPROM Programmer Wizard</h1>
        
        <div class="connection-info">
            <strong>Connected to:</strong> EEPROM_Programmer<br>
            <strong>Password:</strong> dupa1234<br>
            <strong>I2C Pins:</strong> SDA=GPIO5, SCL=GPIO4<br>
            <strong>IP:</strong> 192.168.4.1
        </div>

        <div class="memory-warning">
            ⚠️ <strong>Memory Optimized:</strong> Large HEX files are streamed in chunks to avoid memory issues
        </div>

        <div class="step-indicator">
            <div class="step-dot active">1</div>
            <div class="step-dot">2</div>
            <div class="step-dot">3</div>
        </div>
        
        <div id="step1" class="step active">
            <h2>Step 1: EEPROM Setup</h2>
            <p>Detect and prepare your EEPROM for programming.</p>
            <button class="btn" onclick="detectEEPROM()">Detect EEPROM</button>
            <button class="btn btn-danger" onclick="eraseEEPROM()">Erase EEPROM</button>
            <button class="btn" onclick="toggleWP(false)">WP Disable</button>
            <button class="btn" onclick="toggleWP(true)">WP Enable</button>
            <button class="btn" onclick="doI2CScan()">I2C Scan</button>
            <button class="btn" onclick="openTestWriteDialog()">Test Write</button>
            <label style="margin-left:10px;">Verification:</label>
            <input type="checkbox" id="verifyToggle" onchange="toggleVerification(this.checked)" checked />
            <button class="btn" onclick="setDspRun(true)">DSP Run</button>
            <button class="btn" onclick="setDspRun(false)">DSP Stop</button>
            <div id="eepromStatus" class="status info">Click Detect EEPROM to begin</div>
        </div>

        <div id="step2" class="step">
            <h2>Step 2: Upload HEX File</h2>
            <p>Select your Intel HEX file to program (streaming enabled):</p>
            <div class="file-input">
                <input type="file" id="hexFile" accept=".hex,.txt">
            </div>
            <button class="btn" id="uploadBtn" onclick="uploadHexStream()" disabled>Upload & Program</button>
            <div class="progress">
                <div id="uploadProgress" class="progress-bar">0%</div>
            </div>
            <div id="uploadStatus" class="status info">Select a HEX file to begin</div>
        </div>

        <div id="step3" class="step">
            <h2>Step 3: Verification</h2>
            <p>Verify the programming and test the EEPROM.</p>
            <button class="btn" onclick="verifyEEPROM()">Verify Programming</button>
            <button class="btn" onclick="readEEPROM()">Read EEPROM</button>
            <button class="btn" onclick="dumpEEPROM()">Download Dump</button>
            <div id="verifyStatus" class="status info">Use tools to verify programming</div>
        </div>

        <div>
            <h3>Activity Log</h3>
            <div id="log" class="log">System ready - Streaming enabled for large files</div>
        </div>

        <div>
            <h3>I2C Log</h3>
            <div id="i2clog" class="log">No I2C activity yet</div>
        </div>

        <div style="text-align: center; margin-top: 20px;">
            <button class="btn" id="prevBtn" onclick="prevStep()" disabled>Previous</button>
            <button class="btn" id="nextBtn" onclick="nextStep()">Next</button>
        </div>
    </div>

    <script>
        let currentStep = 1;
        let eepromDetected = false;
        let hexFile = null;
        let uploadInProgress = false;

        function log(message, type = 'info') {
            const logDiv = document.getElementById('log');
            const timestamp = new Date().toLocaleTimeString();
            const icon = type === 'error' ? '❌' : type === 'success' ? '✅' : 'ℹ️';
            logDiv.innerHTML += `${icon} [${timestamp}] ${message}<br>`;
            logDiv.scrollTop = logDiv.scrollHeight;
        }

        function updateStepIndicator() {
            document.querySelectorAll('.step-dot').forEach((dot, index) => {
                dot.classList.toggle('active', index + 1 === currentStep);
            });
        }

        function updateButtons() {
            document.getElementById('prevBtn').disabled = currentStep === 1;
            document.getElementById('nextBtn').disabled = 
                (currentStep === 1 && !eepromDetected) || 
                (currentStep === 2 && !hexFile);
            updateStepIndicator();
        }

        function showStep(step) {
            document.querySelectorAll('.step').forEach(s => s.classList.remove('active'));
            document.getElementById(`step${step}`).classList.add('active');
            currentStep = step;
            updateButtons();
        }

        function nextStep() {
            if (currentStep < 3) showStep(currentStep + 1);
        }

        function prevStep() {
            if (currentStep > 1) showStep(currentStep - 1);
        }

        async function detectEEPROM() {
            log('Detecting EEPROM...');
            try {
                const response = await fetch('/detect');
                if (!response.ok) throw new Error(`HTTP ${response.status}`);
                const data = await response.json();
                
                if (data.success) {
                    document.getElementById('eepromStatus').innerHTML = 
                        '<div class="success">✅ EEPROM detected successfully!</div>';
                    eepromDetected = true;
                    log('EEPROM detected at address 0x50');
                } else {
                    document.getElementById('eepromStatus').innerHTML = 
                        '<div class="error">❌ EEPROM not detected</div>';
                    log('EEPROM detection failed: ' + data.message, 'error');
                }
            } catch (error) {
                log('Detection error: ' + error.message, 'error');
            }
            updateButtons();
        }

        async function eraseEEPROM() {
            if (!confirm('Erase entire EEPROM? This cannot be undone!')) return;
            
            log('Erasing EEPROM...');
            try {
                const response = await fetch('/erase', { method: 'POST' });
                if (!response.ok) throw new Error(`HTTP ${response.status}`);
                const data = await response.json();
                
                if (data.success) {
                    log('EEPROM erased successfully');
                } else {
                    log('Erase failed: ' + data.message, 'error');
                }
            } catch (error) {
                log('Erase error: ' + error.message, 'error');
            }
        }

        document.getElementById('hexFile').addEventListener('change', function(e) {
            hexFile = e.target.files[0];
            if (hexFile) {
                document.getElementById('uploadBtn').disabled = false;
                document.getElementById('uploadStatus').innerHTML = 
                    '<div class="success">✅ HEX file ready! (' + hexFile.size + ' bytes)</div>';
                log(`HEX file selected: ${hexFile.name} (${hexFile.size} bytes)`);
            }
            updateButtons();
        });

async function uploadHexStream() {
    if (!hexFile) {
        log('No HEX file selected', 'error');
        return;
    }
    
    if (uploadInProgress) {
        log('Upload already in progress', 'warning');
        return;
    }
    
    uploadInProgress = true;
    log(`Starting streaming upload: ${hexFile.name} (${hexFile.size} bytes)`);
    
    const progressBar = document.getElementById('uploadProgress');
    const statusDiv = document.getElementById('uploadStatus');
    const uploadBtn = document.getElementById('uploadBtn');
    
    try {
        uploadBtn.disabled = true;
        statusDiv.innerHTML = '<div class="info">🔄 Reading HEX file... (Please wait)</div>';
        progressBar.style.width = '0%';
        progressBar.textContent = '0%';
        
        // CRITICAL FIX: Read file first to ensure it's fully loaded
        const fileBuffer = await readFileAsArrayBuffer(hexFile);
        log(`File read complete: ${fileBuffer.byteLength} bytes in memory`);
        
        if (fileBuffer.byteLength !== hexFile.size) {
            throw new Error(`File size mismatch: expected ${hexFile.size}, got ${fileBuffer.byteLength}`);
        }
        
        statusDiv.innerHTML = '<div class="info">🔄 Uploading to EEPROM programmer... (Do not close browser)</div>';
        
        // Use XMLHttpRequest for better control and progress tracking
        return new Promise((resolve, reject) => {
            const xhr = new XMLHttpRequest();
            const formData = new FormData();
            
            // Convert ArrayBuffer back to File for upload
            const file = new File([fileBuffer], hexFile.name, { type: 'application/octet-stream' });
            formData.append('file', file);
            
            // Add progress tracking for UPLOAD (client -> server)
            xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    log(`Upload transfer: ${e.loaded}/${e.total} bytes (${percent}%)`);
                }
            };
            
            // Track actual bytes sent
            let lastBytesSent = 0;
            const uploadProgressInterval = setInterval(() => {
                if (xhr.upload && xhr.upload.bytesSent > lastBytesSent) {
                    lastBytesSent = xhr.upload.bytesSent;
                    log(`Bytes sent to server: ${lastBytesSent}`);
                }
            }, 500);
            
            // Start server-side progress polling
            let serverProgressInterval = setInterval(async () => {
                try {
                    const resp = await fetch('/progress');
                    const p = await resp.json();
                    const bw = p.bytesWritten || 0;
                    const bt = p.bytesTotal || hexFile.size;
                    
                    if (bt > 0) {
                        const pct = Math.min(100, Math.round((bw / bt) * 100));
                        progressBar.style.width = pct + '%';
                        progressBar.textContent = pct + '%';
                        
                        // Only log progress changes to avoid spam
                        if (pct % 10 === 0 || pct === 100) {
                            log(`EEPROM programming: ${bw}/${bt} bytes (${pct}%)`);
                        }
                    }
                } catch (err) {
                    console.error('Progress poll failed', err);
                }
            }, 1000);
            
            xhr.onload = () => {
                clearInterval(uploadProgressInterval);
                clearInterval(serverProgressInterval);
                
                if (xhr.status >= 200 && xhr.status < 300) {
                    try {
                        const data = JSON.parse(xhr.responseText);
                        handleUploadResponse(data);
                        resolve(data);
                    } catch (parseError) {
                        log(`Failed to parse response: ${parseError.message}`, 'error');
                        reject(parseError);
                    }
                } else {
                    log(`Upload HTTP error: ${xhr.status} ${xhr.statusText}`, 'error');
                    reject(new Error(`HTTP ${xhr.status}: ${xhr.statusText}`));
                }
            };
            
            xhr.onerror = () => {
                clearInterval(uploadProgressInterval);
                clearInterval(serverProgressInterval);
                log('Upload connection failed', 'error');
                reject(new Error('Network error'));
            };
            
            xhr.ontimeout = () => {
                clearInterval(uploadProgressInterval);
                clearInterval(serverProgressInterval);
                log('Upload timeout', 'error');
                reject(new Error('Timeout'));
            };
            
            // No timeout for large files
            xhr.timeout = 0;
            
            log('Starting file transfer to server...');
            xhr.open('POST', `/upload_stream?size=${hexFile.size}`);
            xhr.send(formData);
        });
        
    } catch (error) {
        statusDiv.innerHTML = `<div class="error">❌ Upload error: ${error.message}</div>`;
        progressBar.style.width = '0%';
        progressBar.textContent = '0%';
        log(`Upload error: ${error.message}`, 'error');
        console.error('Upload error details:', error);
    } finally {
        uploadInProgress = false;
        uploadBtn.disabled = false;
    }
}

// Helper function to read file as ArrayBuffer
function readFileAsArrayBuffer(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (e) => resolve(e.target.result);
        reader.onerror = (e) => reject(new Error('File read failed'));
        reader.readAsArrayBuffer(file);
    });
}

// Handle server response
function handleUploadResponse(data) {
    const progressBar = document.getElementById('uploadProgress');
    const statusDiv = document.getElementById('uploadStatus');
    
    if (data.success) {
        progressBar.style.width = '100%';
        progressBar.textContent = '100%';
        statusDiv.innerHTML = '<div class="success">✅ HEX file programmed successfully!</div>';
        
        const bw = data.bytesWritten || 0;
        const bt = data.bytesTotal || 0;
        const parsed = data.parsedBytes || 0;
        
        log(`Programming completed! ${bw} bytes written to EEPROM`, 'success');
        log(`Parser processed: ${parsed} bytes from HEX file`);
        log(`Expected total: ${bt} bytes`);
        
        // Check if we got the full file
        if (parsed < bt * 0.8) {
            log(`⚠️ Warning: Only received ${parsed}/${bt} bytes (${Math.round(parsed/bt*100)}%)`, 'warning');
        }
        
        // Auto-advance to verification step
        setTimeout(() => {
            showStep(3);
            log('Ready for verification step');
        }, 1000);
    } else {
        statusDiv.innerHTML = `<div class="error">❌ Upload failed: ${data.message}</div>`;
        log(`Upload failed: ${data.message}`, 'error');
    }
}

        // FIXED: Better file input handler
        document.getElementById('hexFile').addEventListener('change', function(e) {
            const file = e.target.files[0];
            if (file) {
                // Validate file type
                const fileName = file.name.toLowerCase();
                if (!fileName.endsWith('.hex') && !fileName.endsWith('.txt')) {
                    log('Please select a .hex or .txt file', 'error');
                    e.target.value = '';
                    return;
                }
                
                hexFile = file;
                document.getElementById('uploadBtn').disabled = false;
                document.getElementById('uploadStatus').innerHTML = 
                    `<div class="success">✅ HEX file ready! (${file.size} bytes)</div>`;
                log(`HEX file selected: ${file.name} (${file.size} bytes)`);
                
                // Auto-advance to upload step
                showStep(2);
            } else {
                hexFile = null;
                document.getElementById('uploadBtn').disabled = true;
                document.getElementById('uploadStatus').innerHTML = 
                    '<div class="info">Select a HEX file to begin</div>';
            }
            updateButtons();
        });

        // Toggle write-protect via API
        async function toggleWP(enable) {
            try {
                const response = await fetch('/wp', { method: 'POST', body: JSON.stringify({ enable }) });
                const j = await response.json();
                if (j.success) log('WP set: ' + enable, 'info'); else log('WP change failed', 'error');
            } catch (err) { log('WP request error: ' + err.message, 'error'); }
        }

        // Set DSP run/stop via API
        async function setDspRun(run) {
            try {
                const response = await fetch('/dsp_run', { method: 'POST', body: JSON.stringify({ run }) });
                const j = await response.json();
                if (j.success) log('DSP run set: ' + run, 'info'); else log('DSP change failed', 'error');
            } catch (err) { log('DSP request error: ' + err.message, 'error'); }
        }

        // Legacy upload for small files (keep as fallback)
        async function uploadHex() {
            if (!hexFile) {
                log('No HEX file selected', 'error');
                return;
            }
            
            if (hexFile.size > 10000) {
                log('File too large for direct upload, use streaming instead', 'error');
                return;
            }
            
            log('Uploading small HEX file directly...');
            const reader = new FileReader();
            reader.onload = async function(e) {
                try {
                    const response = await fetch('/upload', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ hex: e.target.result })
                    });
                    const data = await response.json();
                    // Handle response...
                } catch (error) {
                    log('Upload error: ' + error.message, 'error');
                }
            };
            reader.readAsText(hexFile);
        }

        async function verifyEEPROM() {
            log('Verifying EEPROM...');
            try {
                const response = await fetch('/verify');
                if (!response.ok) throw new Error(`HTTP ${response.status}`);
                const data = await response.json();
                
                if (data.success) {
                    document.getElementById('verifyStatus').innerHTML = 
                        '<div class="success">✅ Verification passed!</div>';
                    log('EEPROM verification successful', 'success');
                } else {
                    document.getElementById('verifyStatus').innerHTML = 
                        '<div class="error">❌ Verification failed</div>';
                    log('Verification failed: ' + data.message, 'error');
                }
            } catch (error) {
                log('Verification error: ' + error.message, 'error');
            }
        }

async function readEEPROM() {
    log('Reading first 256 bytes from EEPROM...', 'info');
    try {
        const response = await fetch('/read');
        const data = await response.json();
        if (data.success) {
            log('EEPROM read successful - displaying data:', 'success');
            
            // Parse and display the hex data in a readable format
            const lines = data.data.split('\n');
            lines.forEach((line, index) => {
                if (line.trim()) {
                    const address = index * 16;
                    log(`0x${address.toString(16).padStart(4, '0')}: ${line}`, 'info');
                }
            });
        } else {
            log('EEPROM read failed: ' + data.message, 'error');
        }
    } catch (error) {
        log('Read error: ' + error.message, 'error');
    }
}

async function dumpEEPROM() {
    log('Starting EEPROM dump download...', 'info');
    try {
        const response = await fetch('/dump');
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'eeprom_dump_' + new Date().toISOString().replace(/[:.]/g, '-') + '.bin';
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            log('EEPROM dump downloaded successfully', 'success');
        } else {
            throw new Error(`HTTP ${response.status}`);
        }
    } catch (error) {
        log('Dump error: ' + error.message, 'error');
    }
}


        // Initialize
        async function initialize() {
            updateButtons();
            log('EEPROM Programmer ready - Streaming upload enabled');
            
            try {
                const heapInfo = await getHeapInfo();
                log(`System memory: ${heapInfo} bytes free`);
            } catch (error) {
                log('System initialized with streaming support');
            }
        }

        // Enhanced heap info with timeout
        async function getHeapInfo() {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 3000);
                
                const response = await fetch('/heap', { signal: controller.signal });
                clearTimeout(timeoutId);
                
                const data = await response.json();
                return data.free_heap;
            } catch (error) {
                return 'Unknown';
            }
        }

        // Poll I2C log periodically
        async function pollI2CLog() {
            try {
                const resp = await fetch('/i2c_log');
                const data = await resp.json();
                if (data.log) {
                    const el = document.getElementById('i2clog');
                    el.textContent = data.log;
                    el.scrollTop = el.scrollHeight;
                }
            } catch (err) {
                // ignore
            }
        }
        setInterval(pollI2CLog, 1000);

        async function doI2CScan() {
            log('Running I2C scan...');
            try {
                const resp = await fetch('/i2c_scan');
                const j = await resp.json();
                log('I2C scan result: ' + j.found);
            } catch (err) { log('I2C scan failed: ' + err.message, 'error'); }
        }

        function openTestWriteDialog() {
            const addr = prompt('Enter address (decimal) for test write', '0');
            if (addr === null) return;
            const val = prompt('Enter value (0-255) to write', '85');
            if (val === null) return;
            testWrite(parseInt(addr), parseInt(val));
        }

        async function testWrite(address, value) {
            log(`Test write: address=${address} value=${value}`);
            try {
                const resp = await fetch('/test_write', { method: 'POST', body: JSON.stringify({ address, value }) });
                const j = await resp.json();
                if (j.success) log(`Test write succeeded at ${j.address} = 0x${j.value.toString(16)}`, 'success');
                else log('Test write failed', 'error');
            } catch (err) { log('Test write error: ' + err.message, 'error'); }
        }

        async function toggleVerification(enabled) {
            log('Setting verification: ' + enabled);
            try {
                const resp = await fetch('/verification', { method: 'POST', body: JSON.stringify({ enabled }) });
                const j = await resp.json();
                if (j.success) log('Verification toggled: ' + j.enabled, 'info'); else log('Verification toggle failed', 'error');
            } catch (err) { log('Verification request error: ' + err.message, 'error'); }
        }
    </script>
</body>
</html>
)rawliteral";

#endif
