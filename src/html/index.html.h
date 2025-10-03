#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char MAIN_PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>EEPROM Programmer</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        *{box-sizing:border-box;margin:0;padding:0}
        body{font-family:system-ui;margin:0;padding:20px;background:#667eea;min-height:100vh}
        .container{max-width:1200px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 5px 15px rgba(0,0,0,0.1)}
        h1{text-align:center;color:#333;margin-bottom:8px;font-size:1.8em}
        .subtitle{text-align:center;color:#666;margin-bottom:20px;font-size:0.9em}
        .grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-bottom:20px}
        @media (max-width:768px){.grid{grid-template-columns:1fr}}
        .card{background:#f8f9fa;padding:15px;border-radius:8px;border-left:3px solid #007cba;margin-bottom:15px}
        .card h2{color:#333;margin-bottom:10px;font-size:1.2em}
        .control-group{display:flex;flex-wrap:wrap;gap:6px;margin:10px 0}
        .btn{background:#007cba;color:white;padding:8px 12px;border:none;border-radius:4px;cursor:pointer;font-size:13px;transition:background 0.2s}
        .btn:hover{background:#005a87}
        .btn:disabled{background:#ccc;cursor:not-allowed}
        .btn-danger{background:#dc3545}
        .btn-danger:hover{background:#c82333}
        .btn-success{background:#28a745}
        .btn-success:hover{background:#218838}
        .btn-warning{background:#ffc107;color:#212529}
        .btn-warning:hover{background:#e0a800}
        .file-input{margin:10px 0;padding:15px;border:2px dashed #007cba;border-radius:6px;text-align:center;background:#f8fdff}
        .file-input input[type="file"]{margin:8px 0;padding:8px;background:white;border:1px solid #ddd;border-radius:3px;width:100%;max-width:300px}
        .progress-container{margin:15px 0;background:#e9ecef;border-radius:8px;overflow:hidden}
        .progress-bar{height:20px;background:#007cba;border-radius:8px;width:0%;transition:width 0.3s;color:white;font-size:11px;display:flex;align-items:center;justify-content:center}
        .status{padding:8px 10px;border-radius:4px;margin:8px 0;font-size:13px}
        .success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}
        .error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}
        .info{background:#d1ecf1;color:#0c5460;border:1px solid #bee5eb}
        .warning{background:#fff3cd;color:#856404;border:1px solid #ffeaa7}
        .log-container{margin-top:20px}
        .log{background:#1a1a1a;color:#00ff00;padding:12px;border-radius:6px;height:200px;overflow-y:auto;font-family:monospace;font-size:12px;line-height:1.3}
        .log-line{margin-bottom:1px}
        .system-info{background:#e3f2fd;padding:12px;border-radius:6px;margin-bottom:15px;border-left:3px solid #2196f3}
        .system-info h3{margin-bottom:8px;color:#1976d2}
        .info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px}
        .info-item{display:flex;justify-content:space-between;padding:3px 0;border-bottom:1px solid #bbdefb;font-size:13px}
        .toggle-group{display:flex;align-items:center;gap:8px;margin:8px 0;font-size:13px}
        .toggle{position:relative;display:inline-block;width:40px;height:20px}
        .toggle input{opacity:0;width:0;height:0}
        .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.3s;border-radius:20px}
        .slider:before{position:absolute;content:"";height:14px;width:14px;left:3px;bottom:3px;background:white;transition:.3s;border-radius:50%}
        input:checked+.slider{background:#007cba}
        input:checked+.slider:before{transform:translateX(20px)}
        .connection-status{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:5px}
        .connected{background:#28a745}
        .disconnected{background:#dc3545}
        .tabs{display:flex;border-bottom:1px solid #dee2e6;margin-bottom:15px}
        .tab{padding:8px 16px;background:none;border:none;cursor:pointer;font-size:14px;color:#6c757d;border-bottom:2px solid transparent}
        .tab.active{color:#007cba;border-bottom:2px solid #007cba}
        .tab-content{display:none}
        .tab-content.active{display:block}
    </style>
</head>
<body>
    <div class="container">
        <h1>EEPROM Programmer</h1>
        <div class="subtitle">Complete EEPROM Programming Solution</div>
        
        <div class="system-info">
            <h3>System Information</h3>
            <div class="info-grid">
                <div class="info-item"><span>WiFi Mode:</span><span id="wifiMode">AP</span></div>
                <div class="info-item"><span>EEPROM Size:</span><span>32KB</span></div>
                <div class="info-item"><span>Free Memory:</span><span id="freeMemory">Loading...</span></div>
                <div class="info-item"><span>DSP Status:</span><span id="dspSystemStatus">Unknown</span></div>
            </div>
        </div>

        <div class="grid">
            <div class="card">
                <h2>EEPROM Control</h2>
                <div class="control-group">
                    <button class="btn" onclick="detectEEPROM()">Detect</button>
                    <button class="btn btn-danger" onclick="eraseEEPROM()">Erase</button>
                    <button class="btn btn-warning" onclick="toggleWP(false)">WP Off</button>
                    <button class="btn btn-success" onclick="toggleWP(true)">WP On</button>
                </div>
                <div class="toggle-group">
                    <label>Verification:</label>
                    <label class="toggle"><input type="checkbox" id="verifyToggle" onchange="toggleVerification(this.checked)" checked><span class="slider"></span></label>
                    <span id="verifyStatus">Enabled</span>
                </div>
                <div id="eepromStatus" class="status info"><span class="connection-status disconnected"></span> EEPROM not detected</div>
            </div>

  <!-- REPLACE the existing DSP Control card with this enhanced version -->
<div class="card">
    <h2>DSP Control (ADAU1701)</h2>
    
    <!-- Status Indicators -->
    <div class="status-group">
        <div class="status" id="dspCoreStatus">
            <span class="connection-status disconnected"></span> Core: Unknown
        </div>
        <div class="status" id="dspClockStatus">
            <span class="connection-status disconnected"></span> Clock: Unknown
        </div>
        <div class="status" id="dspPLLStatus">
            <span class="connection-status disconnected"></span> PLL: Unknown
        </div>
    </div>

    <!-- Core Control Buttons -->
    <div class="control-group">
        <button class="btn btn-success" onclick="dspCoreRun(true)">Core Run</button>
        <button class="btn btn-danger" onclick="dspCoreRun(false)">Core Stop</button>
        <button class="btn btn-warning" onclick="dspSoftReset()">Soft Reset</button>
        <button class="btn" onclick="dspSelfBoot()">Self Boot</button>
    </div>

    <!-- Debug & Monitoring -->
    <div class="control-group">
        <button class="btn" id="i2cDebugBtn" onclick="dspToggleI2CDebug()">I2C Debug: OFF</button>
        <button class="btn" onclick="readDSPStatus()">Refresh Status</button>
        <button class="btn" onclick="dspReadRegisters()">Read Registers</button>
    </div>
</div>

        <div class="card">
            <h2>File Upload & Programming</h2>
            <p>Select HEX or BIN file:</p>
            <div class="file-input">
                <input type="file" id="hexFile" accept=".hex,.bin,.txt,.rom">
                <div style="margin-top:8px;font-size:11px;color:#666">Supports: HEX, Binary, C Arrays</div>
            </div>
            <button class="btn" id="uploadBtn" onclick="uploadHexStream()" disabled>Upload & Program</button>
            <div class="progress-container"><div id="uploadProgress" class="progress-bar">0%</div></div>
            <div id="uploadStatus" class="status info">Select file to begin</div>
        </div>

        <div class="card">
            <h2>Verification & Tools</h2>
            <div class="control-group">
                <button class="btn" onclick="readEEPROM()">Read 256B</button>
                <button class="btn" onclick="dumpEEPROM()">Full Dump</button>
                <button class="btn" onclick="verifyAgainstFile()">Verify File</button>
                <button class="btn" onclick="clearLogs()">Clear Logs</button>
            </div>
            <div class="control-group">
                <button class="btn btn-warning" onclick="stressTest()">Stress Test</button>
                <button class="btn" onclick="readConfigArea()">Read Config</button>
            </div>
        </div>

        <div class="tab-container">
            <div class="tabs">
                <button class="tab active" onclick="switchTab('activity')">Activity Log</button>
                <button class="tab" onclick="switchTab('i2c')">I2C Log</button>
                <button class="tab" onclick="switchTab('system')">System Info</button>
            </div>
            
            <div id="activity-tab" class="tab-content active">
                <div class="log" id="log">
                    <div class="log-line">System initialized - Ready for EEPROM programming</div>
                    <div class="log-line">Select a HEX file and click Upload to begin</div>
                </div>
            </div>
            
            <div id="i2c-tab" class="tab-content">
                <div class="log" id="i2clog">
                    <div class="log-line">No I2C activity yet</div>
                    <div class="log-line">Click 'I2C Scan' to detect devices</div>
                </div>
            </div>
            
            <div id="system-tab" class="tab-content">
                <div class="log" id="systemlog">
                    <div class="log-line">EEPROM Programmer v2.0</div>
                    <div class="log-line">Streaming HEX file support enabled</div>
                    <div class="log-line">Max EEPROM size: 32KB</div>
                    <div class="log-line">I2C Address: 0x50</div>
                    <div class="log-line">DSP Address: 0x34</div>
                </div>
            </div>
        </div>
    </div>

<script>
// ===== ENHANCED DSP CONTROL FUNCTIONS =====

// DSP Core Run/Stop Control
async function dspCoreRun(run) {
    try {
        const res = await fetch('/dsp/core_run', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({run: run})
        });
        const data = await res.json();
        if (data.success) {
            log(`DSP core ${run ? 'started' : 'stopped'}`, 'success');
            readDSPStatus(); // Refresh status display
        } else {
            log(`DSP control failed: ${data.message}`, 'error');
        }
    } catch(e) { 
        log('DSP control error: ' + e.message, 'error'); 
    }
}

// DSP Soft Reset (Core Register Method)
async function dspSoftReset() {
    if (!confirm('Soft reset DSP? This will restart processing.')) return;
    log('Initiating DSP soft reset...', 'warning');
    try {
        const res = await fetch('/dsp/soft_reset', {method: 'POST'});
        const data = await res.json();
        if (data.success) {
            log('DSP soft reset completed', 'success');
            setTimeout(readDSPStatus, 500); // Refresh after reset
        }
    } catch(e) { 
        log('Reset error: ' + e.message, 'error'); 
    }
}

// DSP Self-Boot from EEPROM
async function dspSelfBoot() {
    log('Triggering DSP self-boot from EEPROM...', 'info');
    try {
        const res = await fetch('/dsp/self_boot', {method: 'POST'});
        const data = await res.json();
        if (data.success) {
            log('Self-boot initiated', 'success');
            setTimeout(readDSPStatus, 1000); // Allow time for boot
        }
    } catch(e) { 
        log('Self-boot error: ' + e.message, 'error'); 
    }
}

// Toggle I2C Debug Mode
async function dspToggleI2CDebug() {
    const btn = document.getElementById('i2cDebugBtn');
    const currentlyEnabled = btn.textContent.includes('ON');
    const newState = !currentlyEnabled;
    
    try {
        const res = await fetch('/dsp/i2c_debug', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({enabled: newState})
        });
        const data = await res.json();
        if (data.success) {
            btn.textContent = `I2C Debug: ${newState ? 'ON' : 'OFF'}`;
            btn.className = newState ? 'btn btn-warning' : 'btn';
            log(`I2C debug ${newState ? 'enabled' : 'disabled'}`, 'success');
        }
    } catch(e) { 
        log('I2C debug toggle error: ' + e.message, 'error'); 
    }
}

// Read DSP Status and Update UI
async function readDSPStatus() {
    try {
        const res = await fetch('/dsp/status');
        const data = await res.json();
        
        if (data.success) {
            // Update core status
            const coreElem = document.getElementById('dspCoreStatus');
            coreElem.innerHTML = `<span class="connection-status ${data.core_running ? 'connected' : 'disconnected'}"></span> Core: ${data.core_running ? 'Running' : 'Stopped'}`;
            coreElem.className = `status ${data.core_running ? 'success' : 'warning'}`;
            
            // Update clock status
            const clockElem = document.getElementById('dspClockStatus');
            clockElem.innerHTML = `<span class="connection-status ${data.dck_stable ? 'connected' : 'disconnected'}"></span> Clock: ${data.dck_stable ? 'Stable' : 'Unstable'}`;
            clockElem.className = `status ${data.dck_stable ? 'success' : 'error'}`;
            
            // Update PLL status
            const pllElem = document.getElementById('dspPLLStatus');
            pllElem.innerHTML = `<span class="connection-status ${data.pll_locked ? 'connected' : 'disconnected'}"></span> PLL: ${data.pll_locked ? 'Locked' : 'Unlocked'}`;
            pllElem.className = `status ${data.pll_locked ? 'success' : 'warning'}`;
            
        } else {
            log('DSP status read failed', 'error');
        }
    } catch(e) { 
        console.error('Status read error:', e);
    }
}

async function runDSPDiagnostic() {
    log('Running comprehensive DSP diagnostic...', 'info');
    try {
        const res = await fetch('/dsp/diagnostic');
        const data = await res.json();
        
        log(`=== DSP DIAGNOSTIC RESULTS ===`, 'info');
        log(`Basic I2C Detection: ${data.basic_detection ? '✅ SUCCESS' : '❌ FAILED'} (result code: ${data.detect_result})`, 
            data.basic_detection ? 'success' : 'error');
        
        if (data.hardware_id !== undefined) {
            log(`Hardware ID: 0x${data.hardware_id.toString(16).padStart(2, '0')} ${data.hardware_valid ? '✅ (VALID ADAU1701)' : '❌ (INVALID - expected 0x02)'}`, 
                data.hardware_valid ? 'success' : 'error');
        }
        
        if (data.write_test !== undefined) {
            log(`Register Write Test: ${data.write_test ? '✅ SUCCESS' : '❌ FAILED'} (result: ${data.write_result})`, 
                data.write_test ? 'success' : 'error');
        }
        
        if (data.control_readback !== undefined) {
            log(`Control Register Readback: 0x${data.control_readback.toString(16).padStart(2, '0')}`, 'info');
        }
        
        log(`Free Heap: ${data.free_heap_start} → ${data.free_heap_end} bytes`, 'info');
        
        if (data.success) {
            log('✅ DSP communication fully functional', 'success');
        } else {
            log('❌ DSP communication issues detected', 'error');
            
            // Provide troubleshooting tips based on results
            if (data.basic_detection && !data.hardware_valid) {
                log('💡 TROUBLESHOOTING: DSP responds but returns wrong hardware ID. Check:', 'warning');
                log('   - I2C address configuration (should be 0x34 for 7-bit)', 'warning');
                log('   - DSP power supply (3.3V stable)', 'warning');
                log('   - DSP reset line (should be HIGH for operation)', 'warning');
            }
        }
        
    } catch(e) {
        log('❌ DSP diagnostic error: ' + e.message, 'error');
    }
}

// Read DSP Registers for Debugging
async function dspReadRegisters() {
    log('Reading DSP registers...', 'info');
    try {
        const res = await fetch('/dsp/registers');
        const data = await res.json();
        if (data.success) {
            log('DSP registers read successfully', 'success');
            // Display register data in log
            if (data.registers) {
                Object.keys(data.registers).forEach(reg => {
                    log(`REG ${reg}: 0x${data.registers[reg].toString(16).padStart(2, '0')}`, 'info');
                });
            }
        }
    } catch(e) { 
        log('Register read error: ' + e.message, 'error'); 
    }
}

// Update DSP status every 3 seconds when page is active
let dspStatusInterval;
function startDSPStatusPolling() {
    dspStatusInterval = setInterval(readDSPStatus, 3000);
}

function stopDSPStatusPolling() {
    if (dspStatusInterval) {
        clearInterval(dspStatusInterval);
    }
}
let uploadInProgress=false,progressInterval=null;

function log(m,t='info'){
    try{
        const l=document.getElementById('log'),ts=new Date().toLocaleTimeString();
        let i='ℹ️';if(t=='error')i='❌';else if(t=='success')i='✅';else if(t=='warning')i='⚠️';
        const ll=document.createElement('div');ll.className='log-line';ll.innerHTML=`${i} [${ts}] ${m}`;
        l.appendChild(ll);l.scrollTop=l.scrollHeight;console.log(m);
    }catch(e){console.log('LOG ERR:',e,m);}
}

function switchTab(t){
    try{
        document.querySelectorAll('.tab-content').forEach(tab=>tab.classList.remove('active'));
        document.querySelectorAll('.tab').forEach(tab=>tab.classList.remove('active'));
        document.getElementById(`${t}-tab`).classList.add('active');
        document.querySelector(`.tab[onclick="switchTab('${t}')"]`).classList.add('active');
        if(t==='i2c')pollI2CLog();
    }catch(e){console.error('Tab err:',e);}
}

async function detectEEPROM(){
    log('Detecting EEPROM...');
    try{
        const r=await fetch('/detect'),d=await r.json(),s=document.getElementById('eepromStatus');
        if(d.success){
            s.innerHTML='<span class="connection-status connected"></span> EEPROM detected!';
            s.className='status success';log('EEPROM detected','success');
        }else{
            s.innerHTML='<span class="connection-status disconnected"></span> EEPROM not found';
            s.className='status error';log('EEPROM detect fail: '+d.message,'error');
        }
    }catch(e){log('Detect err: '+e.message,'error');}
}

async function eraseEEPROM(){
    if(!confirm('⚠️ ERASE ENTIRE EEPROM?\n\nThis cannot be undone!'))return;
    log('Erasing EEPROM...','warning');
    try{
        const r=await fetch('/erase',{method:'POST'}),d=await r.json();
        if(d.success)log('EEPROM erased','success');else log('Erase fail: '+d.message,'error');
    }catch(e){log('Erase err: '+e.message,'error');}
}

function formatFileSize(b){
    if(b===0)return'0B';if(b<1024)return b+'B';
    else if(b<1048576)return(b/1024).toFixed(1)+'KB';
    else return(b/1048576).toFixed(1)+'MB';
}

async function uploadHexStream(){
    const f=document.getElementById('hexFile').files[0];
    if(!f){log('No file','error');return;}
    if(uploadInProgress){log('Upload in progress','warning');return;}
    uploadInProgress=true;
    const b=document.getElementById('uploadBtn'),p=document.getElementById('uploadProgress'),s=document.getElementById('uploadStatus');
    try{
        b.disabled=true;b.innerHTML='Uploading...';s.innerHTML='<div class="info">Starting upload...</div>';p.style.width='0%';p.textContent='0%';
        log(`Upload: ${f.name} (${formatFileSize(f.size)})`);startProgressPolling();
        const fd=new FormData();fd.append('file',f);
        const r=await fetch(`/upload_stream?size=${f.size}`,{method:'POST',body:fd});
        clearInterval(progressInterval);uploadInProgress=false;b.disabled=false;b.innerHTML='Upload & Program';
        if(r.ok){const d=await r.json();handleUploadResponse(d);}else throw new Error(`HTTP ${r.status}`);
    }catch(e){
        clearInterval(progressInterval);uploadInProgress=false;b.disabled=false;b.innerHTML='Upload & Program';
        s.innerHTML=`<div class="error">Upload err: ${e.message}</div>`;log(`Upload err: ${e.message}`,'error');
    }
}

function startProgressPolling(){
    progressInterval=setInterval(async()=>{
        try{
            const r=await fetch('/progress'),p=await r.json();
            const w=p.bytesWritten||0,t=p.bytesTotal||1,per=Math.min(100,Math.round((w/t)*100));
            const pb=document.getElementById('uploadProgress');pb.style.width=per+'%';pb.textContent=per+'%';
        }catch(e){console.error('Progress poll fail:',e);}
    },1000);
}

function handleUploadResponse(d){
    const s=document.getElementById('uploadStatus'),p=document.getElementById('uploadProgress');
    if(d.success){
        p.style.width='100%';p.textContent='100%';s.innerHTML='<div class="success">Programming done!</div>';
        log(`Programming: ${d.bytesWritten} bytes`,'success');
    }else{
        s.innerHTML=`<div class="error">Programming fail: ${d.message}</div>`;log(`Programming fail: ${d.message}`,'error');
    }
}

async function toggleWP(e){
    try{
        const r=await fetch('/wp',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enable:e})});
        const d=await r.json();if(d.success)log(`WP ${e?'on':'off'}`,'success');else log('WP fail','error');
    }catch(e){log('WP err: '+e.message,'error');}
}

async function setDspRun(r){
    try{
        const res=await fetch('/dsp_run',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({run:r})});
        const d=await res.json();if(d.success)log(`DSP ${r?'start':'stop'}`,'success');else log('DSP fail','error');
    }catch(e){log('DSP err: '+e.message,'error');}
}

async function toggleVerification(e){
    try{
        const r=await fetch('/verification',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:e})});
        const d=await r.json();if(d.success){document.getElementById('verifyStatus').textContent=e?'Enabled':'Disabled';log(`Verify ${e?'on':'off'}`,'success');}
    }catch(e){log('Verify err: '+e.message,'error');}
}

async function doI2CScan(){
    log('I2C scan...');try{const r=await fetch('/i2c_scan'),d=await r.json();log(`I2C: ${d.found}`);}catch(e){log('I2C fail: '+e.message,'error');}
}

function openTestWriteDialog(){
    const a=prompt('EEPROM addr (0-32767):','0');if(a===null)return;
    const v=prompt('Value (0-255):','170');if(v===null)return;testWrite(parseInt(a),parseInt(v));
}

async function testWrite(a,v){
    log(`Test: addr=${a} val=${v}`);try{
        const r=await fetch('/test_write',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({address:a,value:v})});
        const d=await r.json();if(d.success)log(`Test ok: 0x${d.value.toString(16)} @ ${d.address}`,'success');else log('Test fail','error');
    }catch(e){log('Test err: '+e.message,'error');}
}

async function updateSystemInfo(){
    try{const r=await fetch('/heap'),d=await r.json();document.getElementById('freeMemory').textContent=d.free_heap+'B';}catch(e){console.error('Sysinfo fail:',e);}
}

async function pollI2CLog(){
    try{
        const r=await fetch('/i2c_log'),d=await r.json();
        if(d.log){const l=document.getElementById('i2clog');if(l){l.innerHTML='';d.log.split('\n').forEach(line=>{if(line.trim()){const ll=document.createElement('div');ll.className='log-line';ll.textContent=line;l.appendChild(ll);}});l.scrollTop=l.scrollHeight;}}
    }catch(e){}
}

function initializeFileInput(){
    try{
        const f=document.getElementById('hexFile');if(f){
            f.addEventListener('change',function(e){
                const file=e.target.files[0],b=document.getElementById('uploadBtn'),s=document.getElementById('uploadStatus');
                if(file){
                    const n=file.name.toLowerCase(),ex=['.hex','.txt','.bin','.rom'],v=ex.some(ext=>n.endsWith(ext));
                    if(!v){s.innerHTML='<div class="error">Invalid file type</div>';this.value='';b.disabled=true;return;}
                    b.disabled=false;s.innerHTML=`<div class="success">Ready: ${file.name} (${formatFileSize(file.size)})</div>`;log(`File: ${file.name} (${formatFileSize(file.size)})`);
                }else{b.disabled=true;s.innerHTML='<div class="info">Select file</div>';}
            });
        }
    }catch(e){console.error('File init err:',e);}
}

async function readEEPROM(){
    log('Read 256B...','info');try{
        const r=await fetch('/read'),d=await r.json();
        if(d.success){log('Read ok','success');const l=d.data.split('\n');l.forEach((line,i)=>{if(line.trim()){const a=i*16;log(`0x${a.toString(16).padStart(4,'0')}: ${line}`,'info');}});}
        else log('Read fail: '+d.message,'error');
    }catch(e){log('Read err: '+e.message,'error');}
}

async function dumpEEPROM(){
    log('Dump...','info');try{
        const r=await fetch('/dump');if(r.ok){
            const b=await r.blob(),u=window.URL.createObjectURL(b),a=document.createElement('a');
            a.href=u;a.download='eeprom_dump_'+new Date().toISOString().replace(/[:.]/g,'-')+'.bin';
            document.body.appendChild(a);a.click();window.URL.revokeObjectURL(u);document.body.removeChild(a);log('Dump ok','success');
        }else throw new Error(`HTTP ${r.status}`);
    }catch(e){log('Dump err: '+e.message,'error');}
}

async function verifyAgainstFile() {
    const fileInput = document.getElementById('hexFile');
    const file = fileInput.files[0];
    
    if (!file) {
        log('Please select a file first for verification', 'error');
        return;
    }
    
    if (uploadInProgress) {
        log('Please wait for current operation to complete', 'warning');
        return;
    }
    
    uploadInProgress = true;
    log(`Starting verification: ${file.name} (${formatFileSize(file.size)})`, 'info');
    
    try {
        const fileBuffer = await readFileAsArrayBuffer(file);
        const totalBytes = fileBuffer.byteLength;
        
        // Safety check
        if (totalBytes > 32768) {
            log('❌ File too large for 32KB EEPROM', 'error');
            uploadInProgress = false;
            return;
        }
        
        let verifiedBytes = 0;
        let errorCount = 0;
        const CHUNK_SIZE = 64; // Smaller chunks for better stability
        
        // First, let's verify just the first few bytes to debug
        log('Debug: Checking first 16 bytes...', 'info');
        const debugChunk = new Uint8Array(fileBuffer, 0, 16);
        let debugHex = '';
        for (let i = 0; i < 16; i++) {
            debugHex += debugChunk[i].toString(16).padStart(2, '0');
        }
        
        try {
            const debugResponse = await fetch('/verify_range', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({
                    start: 0,
                    length: 16,
                    expectedData: debugHex
                })
            });
            
            const debugData = await debugResponse.json();
            log(`Debug result: ${debugData.success ? 'PASS' : 'FAIL'} - ${debugData.message || ''}`, 
                debugData.success ? 'success' : 'error');
                
            if (!debugData.success && debugData.errorDetails) {
                log(`Error details: ${debugData.errorDetails}`, 'error');
            }
        } catch (debugError) {
            log(`Debug check failed: ${debugError.message}`, 'error');
        }
        
        await new Promise(resolve => setTimeout(resolve, 500));
        
        // Now do the full verification with smaller chunks and better error reporting
        for (let start = 0; start < totalBytes; start += CHUNK_SIZE) {
            const chunkSize = Math.min(CHUNK_SIZE, totalBytes - start);
            const chunk = new Uint8Array(fileBuffer, start, chunkSize);
            
            // Convert chunk to hex
            let chunkHex = '';
            for (let i = 0; i < chunkSize; i++) {
                chunkHex += chunk[i].toString(16).padStart(2, '0');
            }
            
            try {
                const response = await fetch('/verify_range', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        start: start,
                        length: chunkSize,
                        expectedData: chunkHex
                    })
                });
                
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }
                
                const data = await response.json();
                
                if (!data.success) {
                    log(`❌ Verification failed at 0x${start.toString(16).padStart(4, '0')}: ${data.message || 'Data mismatch'}`, 'error');
                    if (data.errorDetails) {
                        log(`Error details: ${data.errorDetails}`, 'error');
                    }
                    errorCount++;
                    
                    // For debugging, read back what's actually in EEPROM
                    if (errorCount <= 3) { // Limit to first 3 errors to avoid spam
                        try {
                            const readbackResponse = await fetch(`/read_range?start=${start}&length=8`);
                            const readbackData = await readbackResponse.json();
                            if (readbackData.success) {
                                log(`EEPROM content at 0x${start.toString(16).padStart(4, '0')}: ${readbackData.hexData}`, 'info');
                            }
                        } catch (readbackError) {
                            log(`Could not read EEPROM for debugging: ${readbackError.message}`, 'error');
                        }
                    }
                    
                    // Continue verification to find all errors
                    // break;
                } else {
                    verifiedBytes += chunkSize;
                    const percent = Math.round((verifiedBytes / totalBytes) * 100);
                    
                    // Only log every 25% to reduce spam
                    if (percent % 25 === 0 || start + chunkSize >= totalBytes) {
                        log(`✓ ${percent}% verified (${verifiedBytes}/${totalBytes} bytes)`, 'success');
                    }
                }
                
            } catch (error) {
                log(`❌ Verification error at 0x${start.toString(16).padStart(4, '0')}: ${error.message}`, 'error');
                errorCount++;
            }
            
            // Increased delay for stability
            await new Promise(resolve => setTimeout(resolve, 150));
        }
        
        if (errorCount === 0) {
            log(`✅ File verification passed: ${verifiedBytes} bytes match`, 'success');
        } else {
            log(`❌ File verification failed: ${errorCount} errors found`, 'error');
            log(`Checked ${verifiedBytes}/${totalBytes} bytes`, 'info');
        }
        
    } catch (error) {
        log(`❌ Verification failed: ${error.message}`, 'error');
    } finally {
        uploadInProgress = false;
    }
}
async function readEEPROMRange(start, length) {
    try {
        const response = await fetch(`/read_range?start=${start}&length=${length}`);
        const data = await response.json();
        if (data.success) {
            log(`EEPROM 0x${start.toString(16)}-0x${(start+length-1).toString(16)}: ${data.hexData}`, 'info');
            return data.hexData;
        } else {
            log(`Read range failed: ${data.message}`, 'error');
            return null;
        }
    } catch (error) {
        log(`Read range error: ${error.message}`, 'error');
        return null;
    }
}
async function updateDSPStatus() {
  try {
    const statusElem = document.getElementById('dspStatus');
    if (!statusElem) {
      console.error('DSP status element not found');
      return;
    }
    
    const response = await fetch('/dsp/status');
    if (!response.ok) {
      statusElem.innerHTML = '<div class="status error">Status: HTTP Error</div>';
      return;
    }
    
    const data = await response.json();
    
    if (data.success) {
      statusElem.innerHTML = `
        <div class="status success">
          DSP: ${data.core_running ? 'RUNNING' : 'STOPPED'} | 
          Clock: ${data.dck_stable ? 'STABLE' : 'UNSTABLE'} |
          PLL: ${data.pll_locked ? 'LOCKED' : 'UNLOCKED'}
        </div>
      `;
    } else {
      statusElem.innerHTML = '<div class="status error">DSP: Not Responding</div>';
    }
  } catch (e) {
    console.error('Status update failed:', e);
    const statusElem = document.getElementById('dspStatus');
    if (statusElem) {
      statusElem.innerHTML = '<div class="status error">Status: Update Failed</div>';
    }
  }
}

// Update every 2 seconds
setInterval(updateDSPStatus, 5000);

async function stressTest(){
    if(!confirm('Run I2C stress test?'))return;log('Stress test...','warning');
    try{const r=await fetch('/stress_test?tests=20'),d=await r.json();if(d.success)log(`Stress: ${d.passedTests}/${d.totalTests}`,'success');else log('Stress fail: '+d.message,'error');}catch(e){log('Stress err: '+e.message,'error');}
}

async function readConfigArea(){
    log('Read config...','info');try{
        const r=await fetch('/read_range?start=0x7F00&length=256'),d=await r.json();
        if(d.success){log('Config ok','success');const h=d.hexData;for(let i=0;i<h.length;i+=32){const c=h.substring(i,i+32),a=0x7F00+(i/2);log(`0x${a.toString(16).padStart(4,'0')}: ${c.match(/.{1,2}/g).join(' ')}`);}}
        else log('Config fail: '+d.message,'error');
    }catch(e){log('Config err: '+e.message,'error');}
}

function clearLogs(){
    document.getElementById('log').innerHTML='<div class="log-line">[Cleared]</div>';
    document.getElementById('i2clog').innerHTML='<div class="log-line">[Cleared]</div>';
    log('Logs cleared','info');
}

function readFileAsArrayBuffer(f){
    return new Promise((res,rej)=>{const r=new FileReader();r.onload=e=>res(e.target.result);r.onerror=e=>rej(new Error('File read fail'));r.readAsArrayBuffer(f);});
}

// Test basic DSP communication without polling
function quickTest() {
    fetch('/dsp/diagnostic')
        .then(r => r.json())
        .then(data => console.log('Quick test:', data));
}

// Find correct DSP address
function findDSP() {
    fetch('/dsp/find')
        .then(r => r.json())
        .then(data => console.log('Address scan:', data));
}


// ADD to existing DOMContentLoaded function:
document.addEventListener('DOMContentLoaded',function(){
    console.log('DOM loaded');initializeFileInput();updateSystemInfo();
    setInterval(updateSystemInfo,30000);setInterval(pollI2CLog,10000);
    
    // START DSP STATUS POLLING - ADD THIS LINE
    startDSPStatusPolling();
    
    log('System ready','success');switchTab('activity');
});

window.addEventListener('error',function(e){console.error('Global err:',e.error);});
window.addEventListener('unhandledrejection',function(e){console.error('Promise reject:',e.reason);e.preventDefault();});
</script>
</body>
</html>
)rawliteral";

#endif