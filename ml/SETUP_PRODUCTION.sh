#!/bin/bash
# Setup production với Nginx + Gunicorn

set -e

echo "🚀 Setup production environment..."

USER=$(whoami)
APP_DIR="$HOME/Pilldispenser/ml"
VENV_DIR="$APP_DIR/venv"

# Cài Nginx nếu chưa có
if ! command -v nginx &> /dev/null; then
    echo "📦 Cài Nginx..."
    sudo apt install nginx -y
fi

# Tạo systemd service cho Flask app
echo "🔧 Tạo systemd service..."
sudo tee /etc/systemd/system/flask-app.service > /dev/null <<EOF
[Unit]
Description=Flask ML App
After=network.target

[Service]
User=$USER
Group=$USER
WorkingDirectory=$APP_DIR
Environment="PATH=$VENV_DIR/bin"
ExecStart=$VENV_DIR/bin/gunicorn \\
    --workers 2 \\
    --bind 127.0.0.1:5000 \\
    --timeout 120 \\
    app:app

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Lấy external IP
EXTERNAL_IP=$(curl -s ifconfig.me || hostname -I | awk '{print $1}')

# Tạo Nginx config
echo "🌐 Setup Nginx..."
sudo tee /etc/nginx/sites-available/flask-app > /dev/null <<EOF
server {
    listen 80;
    server_name $EXTERNAL_IP;

    access_log /var/log/nginx/flask-app-access.log;
    error_log /var/log/nginx/flask-app-error.log;

    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
}
EOF

# Enable site
sudo ln -sf /etc/nginx/sites-available/flask-app /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default

# Test Nginx config
sudo nginx -t

# Setup firewall (local) - Optional, GCP có firewall riêng
echo "🔥 Setup local firewall (optional)..."
if command -v ufw &> /dev/null; then
    sudo ufw allow 22/tcp
    sudo ufw allow 80/tcp
    sudo ufw allow 443/tcp
    sudo ufw --force enable
else
    echo "⚠️  ufw không có sẵn. Không sao, GCP đã có firewall riêng."
    echo "   Chỉ cần mở port 80 trong GCP Console là đủ."
fi

# Enable và start services
echo "🔄 Start services..."
sudo systemctl daemon-reload
sudo systemctl enable flask-app
sudo systemctl restart flask-app
sudo systemctl restart nginx

echo "✅ Setup hoàn tất!"
echo ""
echo "📊 Kiểm tra status:"
sudo systemctl status flask-app --no-pager | head -10
echo ""
echo "🌐 App URL: http://$EXTERNAL_IP"
echo ""
echo "⚠️  QUAN TRỌNG: Cần mở port 80 trong Google Cloud Firewall!"
echo "   Vào GCP Console → VPC Network → Firewall Rules"
echo "   Tạo rule: allow-http, port 80, source: 0.0.0.0/0"
echo ""
echo "🧪 Test:"
echo "  curl http://$EXTERNAL_IP/"
