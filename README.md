## Hướng dẫn chạy toàn bộ dự án (MQTT + ML + JS Web)

Dự án gồm các thành phần:
- **Mosquitto MQTT Broker**: trung gian nhận/gửi dữ liệu từ ESP32 qua MQTT.
- **Flask ML Service (Python)**: API ML chạy mô hình `hypertension_model.keras`.
- **Node.js App (API + Web + Socket.IO + MQTT + MongoDB)**: cung cấp API, giao diện web (thư mục `public`) và realtime.

Chỉ cần làm lần lượt các bước dưới đây là có thể chạy full hệ thống.

---

### 0. Chuẩn bị chung

- **OS khuyến nghị**: Ubuntu (VPS/GCP Compute Engine).
- Đã cài:
  - `git`
  - **Python 3**, `pip`, `python3-venv`
  - **Node.js** (LTS) + `npm`
  - **MongoDB** MongoDB Atlas (URI kết nối)

Clone project:

```bash
git clone https://github.com/LamChanKhai/Pilldispenser
cd my-mqtt-app
```

---

### 1. Cài đặt Mosquitto MQTT Broker

#### 1.1. Cài Mosquitto

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients -y

# Kiểm tra
mosquitto -v
```

#### 1.2. Cấu hình Mosquitto cơ bản

```bash
sudo nano /etc/mosquitto/mosquitto.conf
```

Dán nội dung:

```conf
# Listen on all interfaces, port 1883
listener 1883 0.0.0.0

allow_anonymous true

# Log
log_dest file /var/log/mosquitto/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information

# Persistence
persistence true
persistence_location /var/lib/mosquitto/

# Connection settings
max_connections -1
max_inflight_messages 20
max_queued_messages 1000

keepalive_interval 60
```

Tạo thư mục log và phân quyền:

```bash
sudo mkdir -p /var/log/mosquitto
sudo chown mosquitto:mosquitto /var/log/mosquitto
sudo chmod 755 /var/log/mosquitto
```

Bật service Mosquitto:

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
sudo systemctl status mosquitto
```

Test nhanh MQTT:

```bash
# Terminal 1
mosquitto_sub -h localhost -t "pill/data/status" -v

# Terminal 2
mosquitto_pub -h localhost -t "pill/data/status" -m "test message"
```

Nếu Terminal 1 nhận được `"test message"` là OK.

**Firewall GCP**: mở port `1883/TCP` cho MQTT:
- GCP Console → VPC Network → Firewall → CREATE FIREWALL RULE
- Name: `allow-mqtt`
- Port: `1883`
- Source: `0.0.0.0/0`

---

### 2. Deploy Flask ML Service (Python + Nginx + Gunicorn)

Dựa trên script `ml/Deloy_python_ml.sh`.

> Script mặc định dùng đường dẫn: `APP_DIR="$HOME/Pilldispenser/ml"`.  
> Bạn có hai lựa chọn:
> - **Cách 1 (đơn giản)**: copy project vào `~/Pilldispenser` sao cho thư mục ML nằm ở `~/Pilldispenser/ml`.
> - **Cách 2 (tùy biến)**: sửa biến `APP_DIR` trong `ml/Deloy_python_ml.sh` cho khớp với đường dẫn thực tế.

#### 2.1. Chuẩn bị môi trường Python

Ví dụ (theo cấu trúc mặc định của script):

```bash
mkdir -p ~/Pilldispenser
cp -r my-mqtt-app/* ~/Pilldispenser/

cd ~/Pilldispenser/ml

# Tạo venv
python3 -m venv venv
source venv/bin/activate

# Cài các thư viện Python cần thiết (Flask, Gunicorn, TensorFlow/PyTorch, v.v.)
# Nếu bạn có file requirements.txt thì:
# pip install -r requirements.txt
```

Đảm bảo trong thư mục `ml` có Flask app với entry `app:app` (biến `app` trong file `app.py` hoặc tương tự).

#### 2.2. Chạy script deploy

```bash
cd ~/Pilldispenser/ml
chmod +x Deloy_python_ml.sh
./Deloy_python_ml.sh
```

Script sẽ:
- Tạo systemd service `flask-app` chạy Gunicorn bind tới `127.0.0.1:5000`.
- Cấu hình Nginx reverse proxy từ port 80 vào `127.0.0.1:5000`.
- Bật và restart `flask-app` + `nginx`.
- In ra `EXTERNAL_IP` và URL truy cập.

**Firewall GCP**: mở port `80/TCP`:
- GCP Console → VPC Network → Firewall Rules → tạo rule `allow-http` với port `80`, source `0.0.0.0/0`.

Sau đó, có thể truy cập Flask ML API qua:

```bash
curl http://<EXTERNAL_IP>/
```

---

### 3. Chạy Node.js App (API + JS Web + MQTT + MongoDB)

Node.js app nằm ở root project (file `server.js`, `app.js`, thư mục `public`, `routes`, v.v.).

#### 3.1. Cài dependencies

Từ thư mục project:

```bash
cd ~/my-mqtt-app   # hoặc đường dẫn bạn clone
npm install
```

#### 3.2. Cấu hình biến môi trường

App dùng file `config/env.js`, load biến môi trường từ `.env.<NODE_ENV>.local`.  
Khi chạy local development, nên tạo file:

```bash
nano .env.development.local
```

Ví dụ nội dung:

```env
PORT=3000

# MQTT broker
MQTT_BROKER_URL=mqtt://<MQTT_SERVER_IP>:1883

# MongoDB
DB_URI=mongodb://<user>:<password>@<host>:<port>/<db_name>?authSource=admin

# Auth / người dùng mặc định (tuỳ app)
DEFAULT_USER_ID=<optional_user_id>
JWT_SECRET=your_jwt_secret_here

# Gemini (nếu dùng tính năng AI)
GEMINI_API_KEY=<optional_gemini_api_key>
GEMINI_MODEL=<optional_gemini_model_name>
```

Ghi chú:
- `PORT` là port HTTP server của Node.js (mặc định 3000 nếu không set).
- `MQTT_BROKER_URL` phải trỏ tới MQTT broker bạn đã cài (Mosquitto).
- `DB_URI` là connection string MongoDB.

#### 3.3. Chạy server

Chạy development (auto reload bằng `nodemon`):

```bash
npm run dev
```

Chạy production (simple):

```bash
npm start
```

Khi server chạy bạn sẽ thấy log dạng:

```text
🚀 Server running at http://localhost:<PORT>
```

Giao diện web tĩnh nằm trong thư mục `public` và được serve bởi Express.  
Mở trình duyệt:

- Local: `http://localhost:<PORT>`
- Trên VPS: `http://<SERVER_IP>:<PORT>`

**Lưu ý CORS**: Socket.IO trong `server.js` hiện đang cho phép origin `http://localhost:3000`.  
Nếu bạn deploy web ở domain/port khác, cần sửa lại cấu hình CORS trong `server.js`.

---

### 4. Kết nối toàn bộ (End-to-end)

1. **Đảm bảo MongoDB đang chạy** và `DB_URI` chính xác.
2. **Đảm bảo Mosquitto** đã hoạt động (`sudo systemctl status mosquitto`).
3. **Chạy Flask ML Service** (service `flask-app` phải `active (running)`):
   - `sudo systemctl status flask-app`
4. **Chạy Node.js app**:
   - `npm run dev` (local) hoặc `npm start` (prod).
5. **ESP32**:
   - Cấu hình `MQTT_BROKER_URL`/host/port trên ESP32 trỏ tới cùng broker.
6. Mở web (JS web) trên browser và kiểm tra:
   - Dữ liệu realtime qua MQTT → Node.js → Socket.IO.
   - Các API `/api/...` hoạt động, ML predict route `/api/predict` kết nối được tới Flask ML.

---

### 5. Một số lệnh hữu ích

**Mosquitto:**

```bash
sudo systemctl restart mosquitto
sudo tail -f /var/log/mosquitto/mosquitto.log
sudo netstat -tlnp | grep 1883
```

**Flask ML service:**

```bash
sudo systemctl status flask-app
sudo systemctl restart flask-app
sudo systemctl restart nginx
```

**Node.js app:**

```bash
npm run dev      # Dev mode
npm start        # Prod mode đơn giản
```

Chỉ cần làm theo đúng thứ tự 1 → 2 → 3 và đảm bảo firewall (80, 1883, PORT Node.js) đã mở là có thể chạy được toàn bộ dự án, bao gồm cả JS web.

