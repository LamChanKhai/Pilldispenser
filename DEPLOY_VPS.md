# Hướng dẫn deploy lên VPS

Hướng dẫn chi tiết để chạy **my-mqtt-app** trên VPS (Ubuntu/Debian). MQTT và Socket.IO cần process chạy lâu dài nên VPS là lựa chọn phù hợp.

---

## 1. Yêu cầu VPS

- **OS**: Ubuntu 20.04/22.04 hoặc Debian
- **RAM**: tối thiểu 512MB (1GB khuyến nghị)
- **Port** cần mở: `3000` (Node.js), `1883` (MQTT nếu broker trên cùng VPS)

---

## 2. Cài đặt môi trường

### 2.1. Cập nhật hệ thống

```bash
sudo apt update && sudo apt upgrade -y
```

### 2.2. Cài Node.js (LTS)

```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs
node -v   # Kiểm tra: v20.x.x
```

### 2.3. Cài PM2 (quản lý process)

```bash
sudo npm install -g pm2
pm2 -v
```

---

## 3. Deploy ứng dụng

### 3.1. Clone hoặc upload code

**Cách 1: Clone từ Git**

```bash
cd ~
git clone https://github.com/YOUR_USERNAME/my-mqtt-app.git
cd my-mqtt-app
```

**Cách 2: Upload qua SCP**

Từ máy local:

```bash
scp -r ./my-mqtt-app user@YOUR_VPS_IP:~/
```

### 3.2. Cài dependencies

```bash
cd ~/my-mqtt-app
npm install --production
```

### 3.3. Tạo file cấu hình môi trường

```bash
nano .env.production.local
```

Nội dung mẫu (điền giá trị thật):

```env
PORT=3000

# MQTT broker - IP của VPS hoặc server chạy Mosquitto
MQTT_BROKER_URL=mqtt://YOUR_MQTT_IP:1883

# MongoDB Atlas hoặc MongoDB trên VPS
DB_URI=mongodb+srv://user:pass@cluster.mongodb.net/dbname?retryWrites=true&w=majority

# Auth
JWT_SECRET=your_random_secret_key_here
DEFAULT_USER_ID=

# CORS cho Socket.IO - URL mà user truy cập app
# VD: http://YOUR_VPS_IP:3000 hoặc https://yourdomain.com
CORS_ORIGIN=http://YOUR_VPS_IP:3000

# Gemini (tuỳ chọn)
GEMINI_API_KEY=
GEMINI_MODEL=
```

**Quan trọng**: `CORS_ORIGIN` phải là địa chỉ mà trình duyệt dùng để mở app (kể cả `http://IP:3000`).

---

## 4. Chạy với PM2

### 4.1. Tạo thư mục logs

```bash
mkdir -p ~/my-mqtt-app/logs
```

### 4.2. Khởi động app

```bash
cd ~/my-mqtt-app
pm2 start ecosystem.config.cjs
```

### 4.3. Kiểm tra

```bash
pm2 status
pm2 logs my-mqtt-app
```

Nếu thấy `✅ Connected to MQTT broker` và `🚀 Server running at http://localhost:3000` là ổn.

### 4.4. Auto start khi reboot

```bash
pm2 startup
pm2 save
```

---

## 5. Mở firewall

```bash
sudo ufw allow 3000/tcp
sudo ufw allow 22/tcp    # SSH
# Nếu MQTT broker trên VPS:
# sudo ufw allow 1883/tcp
sudo ufw enable
sudo ufw status
```

---

## 6. Truy cập ứng dụng

- **HTTP**: `http://YOUR_VPS_IP:3000`
- Kiểm tra API: `curl http://YOUR_VPS_IP:3000/api/v1/user/...`

---

## 7. Một số lệnh PM2 hữu ích

| Lệnh | Mô tả |
|------|-------|
| `pm2 status` | Xem trạng thái app |
| `pm2 logs my-mqtt-app` | Xem log realtime |
| `pm2 restart my-mqtt-app` | Khởi động lại |
| `pm2 stop my-mqtt-app` | Dừng app |
| `pm2 delete my-mqtt-app` | Xoá khỏi PM2 |

---

## 8. (Tuỳ chọn) Nginx + domain + HTTPS

Nếu có domain trỏ về VPS và muốn dùng port 80/443 với SSL:

### 8.1. Cài Nginx

```bash
sudo apt install nginx -y
```

### 8.2. Tạo file cấu hình

```bash
sudo nano /etc/nginx/sites-available/my-mqtt-app
```

Nội dung (thay `yourdomain.com`):

```nginx
server {
    listen 80;
    server_name yourdomain.com www.yourdomain.com;

    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### 8.3. Bật site và cài SSL (Let's Encrypt)

```bash
sudo ln -s /etc/nginx/sites-available/my-mqtt-app /etc/nginx/sites-enabled/
sudo apt install certbot python3-certbot-nginx -y
sudo certbot --nginx -d yourdomain.com -d www.yourdomain.com
```

### 8.4. Cập nhật CORS

Trong `.env.production.local`:

```env
CORS_ORIGIN=https://yourdomain.com
```

Sau đó restart:

```bash
pm2 restart my-mqtt-app
```

---

## 9. Lưu ý

1. **MQTT Broker**: Nếu Mosquitto chạy trên VPS khác, đảm bảo VPS app có thể kết nối tới port 1883.
2. **MongoDB**: Nên dùng MongoDB Atlas (cloud) hoặc cài MongoDB trên VPS.
3. **CORS_ORIGIN**: Phải trùng với URL mà user mở app để Socket.IO hoạt động.
4. **ESP32**: Cập nhật `mqtt_server` trong `PillDispenser/config.cpp` trỏ tới IP MQTT broker.

---

Nếu gặp lỗi, kiểm tra log: `pm2 logs my-mqtt-app --lines 100`
