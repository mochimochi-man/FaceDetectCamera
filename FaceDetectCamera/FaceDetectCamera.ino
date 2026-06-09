// ============================================
// FaceDetectCamera - MediaPipe Face Detection Server
// WiFi設定を内蔵メモリに保存 + Web設定画面対応版
// 認識結果と顔画像(base64)をJSONでシリアル出力
// ============================================

#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <Preferences.h>

// ---- WiFi設定保存用 ----
Preferences wifiPrefs;
bool configMode = false;
bool serialOut = false;

// XIAO ESP32S3 Sense Camera Pins (OFFICIAL)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

httpd_handle_t camera_httpd = NULL;

// シリアル出力制御マクロ
#define LOG_I(fmt, ...) do { if (serialOut) Serial.printf(fmt, ##__VA_ARGS__); } while(0)
#define LOG_S(s) do { if (serialOut) Serial.println(s); } while(0)

// ============================================
// 通常動作モード: 顔認識Webカメラ画面
// ============================================

static esp_err_t index_handler(httpd_req_t *req) {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32S3 MediaPipe Face Detection</title>
<style>
body{font-family:Arial;background:#fff;color:#333;text-align:center;margin:0}
#container{position:relative;display:inline-block;margin:10px}
#canvas{max-width:100%;width:320px;height:240px;background:#000;border:2px solid #000;border-radius:8px}
#stats{margin:10px;font-size:12px;color:#666}
button{padding:8px 16px;margin:5px;border:none;border-radius:5px;background:#e0e0e0;color:#000;font-weight:bold;cursor:pointer}
#loading{color:#666;font-size:14px;margin:10px}
</style>
<script src="https://cdn.jsdelivr.net/npm/@mediapipe/face_detection@0.4/face_detection.js" crossorigin="anonymous"></script>
</head>
<body>
<div id="loading">Loading MediaPipe model... (first time only, ~2MB)</div>
<div id="stats">Initializing...</div>
<div id="container">
  <canvas id="canvas" width="320" height="240"></canvas>
</div>
<div>
  <button onclick="start()">Start</button>
  <button onclick="stop()">Stop</button>
</div>
<script>
const canvas=document.getElementById('canvas');
const ctx=canvas.getContext('2d');
const stats=document.getElementById('stats');
const loading=document.getElementById('loading');
let faceDetection=null;
let streaming=false;
let isProcessing=false;
let lastTime=0;
let frameCount=0;
let currentBitmap=null;
let lastPostTime=0;
let intervalSec=15;

// 保存された送信間隔を取得
fetch('/interval').then(r=>r.text()).then(v=>{
  intervalSec=parseInt(v)||15;
}).catch(()=>{});

function getTimeObj(){
  const d=new Date();
  return {
    year:d.getFullYear(),
    month:d.getMonth()+1,
    day:d.getDate(),
    hour:d.getHours(),
    minute:d.getMinutes(),
    second:d.getSeconds(),
    ms:d.getMilliseconds(),
    iso:d.toISOString()
  };
}

async function initMediaPipe(){
  faceDetection=new FaceDetection({locateFile:(file)=>{
    return 'https://cdn.jsdelivr.net/npm/@mediapipe/face_detection@0.4/'+file;
  }});
  faceDetection.setOptions({
    model:'short',
    minDetectionConfidence:0.5
  });
  faceDetection.onResults(onResults);
  loading.style.display='none';
  stats.textContent='MediaPipe ready. Click Start.';
}

function getBox(det){
  const b=det.boundingBox;
  if(b){
    if(b.xMin!==undefined) return {xMin:b.xMin,yMin:b.yMin,width:b.width,height:b.height};
    if(b.xCenter!==undefined) return {xMin:b.xCenter-b.width/2,yMin:b.yCenter-b.height/2,width:b.width,height:b.height};
    if(b.xmin!==undefined) return {xMin:b.xmin,yMin:b.ymin,width:b.width,height:b.height};
  }
  const r=det.locationData?.relativeBoundingBox;
  if(r) return {xMin:r.xmin,yMin:r.ymin,width:r.width,height:r.height};
  return null;
}

function getScore(det){
  if(!det) return null;
  // MediaPipe @0.4 compiled protobuf: score is in det.V[0].ga
  if(det.V && det.V.length>0 && typeof det.V[0].ga==='number') return det.V[0].ga;
  // Fallbacks
  if(Array.isArray(det.score) && det.score.length>0 && typeof det.score[0]==='number') return det.score[0];
  if(typeof det.score==='number') return det.score;
  if(det.categories && det.categories.length>0 && typeof det.categories[0].score==='number') return det.categories[0].score;
  return null;
}

function onResults(results){
  if(!streaming) return;
  try{
    const now=performance.now();
    frameCount++;
    if(now-lastTime>=1000){
      stats.textContent='FPS: '+frameCount+' | Faces: '+(results.detections?results.detections.length:0);
      frameCount=0;
      lastTime=now;
    }

    if(currentBitmap){
      ctx.drawImage(currentBitmap,0,0,canvas.width,canvas.height);
    }

    if(results.detections && results.detections.length>0){
      results.detections.forEach((det,i)=>{
        const box=getBox(det);
        if(!box) return;

        const x=box.xMin*canvas.width;
        const y=box.yMin*canvas.height;
        const w=box.width*canvas.width;
        const h=box.height*canvas.height;

        ctx.strokeStyle='#0f0';
        ctx.lineWidth=2;
        ctx.strokeRect(x,y,w,h);

        const sc=getScore(det);
        let scoreText=(typeof sc==='number')?(sc*100).toFixed(0)+'%':'?';
        ctx.fillStyle='#0f0';
        ctx.font='bold 11px Arial';
        ctx.fillText(scoreText,x,y-4);
      });

      // ---- 設定間隔以上経過した時のみPOST ----
      const nowMs=Date.now();
      if(nowMs-lastPostTime>=intervalSec*1000){
        lastPostTime=nowMs;

        // 顔画像を64x64サムネイルでbase64化
        let faceImages=[];
        if(currentBitmap){
          const tmpCanvas=document.createElement('canvas');
          const tmpCtx=tmpCanvas.getContext('2d');
          tmpCanvas.width=64;
          tmpCanvas.height=64;
          results.detections.forEach(d=>{
            const b=getBox(d);
            if(!b) return;
            const sx=b.xMin*currentBitmap.width;
            const sy=b.yMin*currentBitmap.height;
            const sw=b.width*currentBitmap.width;
            const sh=b.height*currentBitmap.height;
            tmpCtx.clearRect(0,0,64,64);
            tmpCtx.drawImage(currentBitmap,sx,sy,sw,sh,0,0,64,64);
            faceImages.push(tmpCanvas.toDataURL('image/jpeg',0.3));
          });
        }

        const faces=results.detections.map(d=>{
          const b=getBox(d);
          const sc=getScore(d);
          return {
            xMin:b?b.xMin:null,
            yMin:b?b.yMin:null,
            width:b?b.width:null,
            height:b?b.height:null,
            score:(typeof sc==='number')?sc:null
          };
        });

        if(streaming){
          fetch('/face',{
            method:'POST',
            headers:{'Content-Type':'application/json'},
            body:JSON.stringify({faces:faces,count:faces.length,time:getTimeObj(),images:faceImages})
          }).catch(()=>{});
        }
      }
    }
  }catch(e){
    console.error('Draw error:',e);
  }

  if(currentBitmap){
    currentBitmap.close();
    currentBitmap=null;
  }

  isProcessing=false;

  if(streaming){
    setTimeout(captureAndProcess,150);
  }
}

async function captureAndProcess(){
  if(!streaming || isProcessing) return;
  isProcessing=true;
  try{
    const res=await fetch('/capture?t='+Date.now());
    const blob=await res.blob();
    currentBitmap=await createImageBitmap(blob);
    await faceDetection.send({image:currentBitmap});
  }catch(e){
    console.error('Capture/Process error:',e);
    if(currentBitmap){
      currentBitmap.close();
      currentBitmap=null;
    }
    isProcessing=false;
    if(streaming){
      setTimeout(captureAndProcess,500);
    }
  }
}

function start(){
  streaming=true;
  lastPostTime=Date.now();
  captureAndProcess();
}
function stop(){
  streaming=false;
  isProcessing=false;
  if(currentBitmap){
    currentBitmap.close();
    currentBitmap=null;
  }
  ctx.clearRect(0,0,canvas.width,canvas.height);
  stats.textContent='Stopped';
}

initMediaPipe();
</script>
</body>
</html>
)rawliteral";
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, html, strlen(html));
}

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  uint8_t* jpg_buf = NULL;
  size_t jpg_len = 0;
  bool converted = frame2jpg(fb, 85, &jpg_buf, &jpg_len);
  esp_camera_fb_return(fb);

  if (!converted || !jpg_buf || jpg_len == 0) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  esp_err_t res = httpd_resp_send(req, (const char*)jpg_buf, jpg_len);
  free(jpg_buf);
  return res;
}

static esp_err_t status_handler(httpd_req_t *req) {
  char json[512];
  snprintf(json, sizeof(json),
    "{\"heap\":%d,\"psram\":%d,\"uptime\":%lu}",
    ESP.getFreeHeap(), ESP.getFreePsram(), millis()/1000);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t interval_handler(httpd_req_t *req) {
  wifiPrefs.begin("wificfg", true);
  int interval = wifiPrefs.getInt("interval", 15);
  wifiPrefs.end();
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", interval);
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, buf, strlen(buf));
}

// ============================================
// 顔認識結果受信: JSONをシリアルに出力
// ============================================

static esp_err_t face_handler(httpd_req_t *req) {
  int total_len = req->content_len;
  if (total_len <= 0) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":false}", 13);
  }
  if (total_len > 8192) total_len = 8192;

  char *buf = (char*)malloc(total_len + 1);
  if (!buf) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
    return ESP_FAIL;
  }

  int received = 0;
  while (received < total_len) {
    int ret = httpd_req_recv(req, buf + received, total_len - received);
    if (ret <= 0) break;
    received += ret;
  }
  buf[received] = '\0';

  if (received > 0) {
    Serial.println(buf);
  }

  free(buf);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, "{\"ok\":true}", 11);
}

// ============================================
// 設定モード: WiFi設定画面
// ============================================

static esp_err_t config_page_handler(httpd_req_t *req) {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>WiFi設定</title>
<style>
body{font-family:Arial,Meiryo,sans-serif;background:#f5f5f5;color:#333;text-align:center;margin:0;padding:20px}
.box{background:#fff;padding:25px;border-radius:12px;max-width:340px;margin:40px auto;box-shadow:0 4px 12px rgba(0,0,0,0.1)}
h2{margin:0 0 15px 0;font-size:1.3em}
input{width:100%;padding:12px;margin:8px 0;border:1px solid #ccc;border-radius:6px;box-sizing:border-box;font-size:1em}
button{width:100%;padding:12px;background:#000;color:#fff;border:none;border-radius:6px;font-weight:bold;font-size:1em;cursor:pointer;margin-top:10px}
.note{font-size:0.85em;color:#666;margin-top:15px}
.row{display:flex;gap:10px;align-items:center;margin:8px 0}
.row label{white-space:nowrap;font-size:0.95em}
.row input{flex:1;width:auto;margin:0}
</style>
</head>
<body>
<div class="box">
<h2>WiFi設定</h2>
<p style="font-size:0.9em;color:#444;margin-top:0">保存されたWiFi設定がありません。<br>接続先WiFiを設定してください。</p>
<form action="/save" method="GET">
<input type="text" name="ssid" placeholder="SSID" required maxlength="32">
<input type="password" name="pass" placeholder="Password" required maxlength="64">
<div class="row">
  <label>送信間隔:</label>
  <input type="number" name="interval" value="15" min="1" max="300">
  <label>秒</label>
</div>
<button type="submit">保存して再起動</button>
</form>
</div>
</body>
</html>
)rawliteral";
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, html, strlen(html));
}

static esp_err_t save_config_handler(httpd_req_t *req) {
  char ssid[64] = {0};
  char pass[64] = {0};
  char intervalStr[16] = "15";

  size_t buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    char *query = (char*)malloc(buf_len);
    if (query && httpd_req_get_url_query_str(req, query, buf_len) == ESP_OK) {
      httpd_query_key_value(query, "ssid", ssid, sizeof(ssid));
      httpd_query_key_value(query, "pass", pass, sizeof(pass));
      httpd_query_key_value(query, "interval", intervalStr, sizeof(intervalStr));
    }
    if (query) free(query);
  }

  if (strlen(ssid) > 0) {
    int interval = atoi(intervalStr);
    if (interval < 1) interval = 1;
    if (interval > 300) interval = 300;

    wifiPrefs.begin("wificfg", false);
    wifiPrefs.putString("ssid", ssid);
    wifiPrefs.putString("pass", pass);
    wifiPrefs.putInt("interval", interval);
    wifiPrefs.end();

    const char* msg = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head>"
                      "<body style='text-align:center;font-family:Arial;padding:40px'>"
                      "<h2>設定を保存しました</h2><p>再起動します...</p></body></html>";
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, msg, strlen(msg));

    delay(500);
    ESP.restart();
    return ESP_OK;
  }

  httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid parameters");
  return ESP_FAIL;
}

// ============================================
// Server Start
// ============================================

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;
  config.stack_size = 16384;

  httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL };
  httpd_uri_t capture_uri = { .uri = "/capture", .method = HTTP_GET, .handler = capture_handler, .user_ctx = NULL };
  httpd_uri_t status_uri = { .uri = "/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL };
  httpd_uri_t face_uri = { .uri = "/face", .method = HTTP_POST, .handler = face_handler, .user_ctx = NULL };
  httpd_uri_t interval_uri = { .uri = "/interval", .method = HTTP_GET, .handler = interval_handler, .user_ctx = NULL };

  LOG_I("Starting server on port: '%d'\n", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &face_uri);
    httpd_register_uri_handler(camera_httpd, &interval_uri);
    LOG_S("Server started!");
  }
}

void startConfigServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;
  config.stack_size = 8192;

  httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = config_page_handler, .user_ctx = NULL };
  httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_GET, .handler = save_config_handler, .user_ctx = NULL };

  LOG_I("Starting config server on port: '%d'\n", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &save_uri);
    LOG_S("Config server started!");
  }
}

// ============================================
// Camera Init
// ============================================

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    LOG_I("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    LOG_I("Camera PID: 0x%04X\n", s->id.PID);
    s->set_vflip(s, 1);
    s->set_brightness(s, 0);
    s->set_contrast(s, 1);
  }
  return true;
}

// ============================================
// Setup & Loop
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 内蔵メモリからWiFi設定を読み出し
  wifiPrefs.begin("wificfg", true);
  String savedSSID = wifiPrefs.getString("ssid", "");
  String savedPass = wifiPrefs.getString("pass", "");
  wifiPrefs.end();

  // 設定がなければ設定モード
  if (savedSSID.length() == 0) {
    configMode = true;
    serialOut = true;
  }

  if (configMode) {
    // ---- 設定モード ----
    LOG_S("\n========================================");
    LOG_S("  WiFi Configuration Mode");
    LOG_S("========================================");

    WiFi.softAP("Camera-Setup", "setup1234");
    IPAddress IP = WiFi.softAPIP();

    LOG_S("Access Point started. Connect to:");
    LOG_S("  SSID: Camera-Setup");
    LOG_S("  Password: setup1234");
    LOG_I("  Config URL: http://%s/\n", IP.toString().c_str());

    startConfigServer();

  } else {
    // ---- 通常動作モード ----
    Serial.setDebugOutput(false);

    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    WiFi.setSleep(false);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    if (!initCamera()) {
      while (1) delay(1000);
    }

    startCameraServer();
  }
}

void loop() {
  delay(10000);
}
