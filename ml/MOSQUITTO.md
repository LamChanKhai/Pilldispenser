# Cài đặt và Migrate Mosquitto MQTT Broker

## Bước 1: Cài đặt Mosquitto

```bash
# Update system
sudo apt update

# Cài Mosquitto broker và client tools
sudo apt install mosquitto mosquitto-clients -y

# Verify installation
mosquitto -v
```

## Bước 2: Cấu hình Mosquitto


### Tạo config file

```bash
sudo nano /etc/mosquitto/mosquitto.conf
```

Dùng config **không có `include_dir`** (để tránh load file bridge lỗi):

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


## Bước 3: Tạo thư mục log và set permissions

```bash
sudo mkdir -p /var/log/mosquitto
sudo chown mosquitto:mosquitto /var/log/mosquitto
sudo chmod 755 /var/log/mosquitto
```

## Bước 4: Test Mosquitto

```bash
# Test config
sudo mosquitto -c /etc/mosquitto/mosquitto.conf -v

# Nếu OK, Ctrl+C để dừng
```

## Bước 5: Tạo Systemd Service

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
sudo systemctl status mosquitto
```

## Bước 6: Test MQTT Connection

### Terminal 1: Subscribe
```bash
mosquitto_sub -h localhost -t "pill/data/status" -v
```

### Terminal 2: Publish
```bash
mosquitto_pub -h localhost -t "pill/data/status" -m "test message"
```

Nếu thấy message ở Terminal 1 → OK!

## Bước 7: Mở Port 1883 trong GCP Firewall

### Qua Console:
1. Vào GCP Console → VPC Network → Firewall
2. CREATE FIREWALL RULE:
   - **Name:** `allow-mqtt`
   - **Port:** `1883` (TCP)
   - **Source:** `0.0.0.0/0` 
   - **CREATE**



## Bước 8: Lấy External IP của VPS mới

```bash
curl ifconfig.me
# Hoặc
hostname -I
```


## Bước 9: Test từ ESP32

1. Upload code mới lên ESP32 với IP mới
2. Check logs trong ESP32 Serial Monitor
3. Check Mosquitto logs:

```bash
sudo tail -f /var/log/mosquitto/mosquitto.log
```


## Lệnh hữu ích

```bash
# Restart Mosquitto
sudo systemctl restart mosquitto

# Xem logs
sudo tail -f /var/log/mosquitto/mosquitto.log

# List connected clients
sudo mosquitto_sub -h localhost -t '$SYS/#' -v

# Check port đang listen
sudo netstat -tlnp | grep 1883
```
