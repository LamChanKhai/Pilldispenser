/**
 * PM2 ecosystem config - dùng để chạy app trên VPS
 * Cài PM2: npm install -g pm2
 * Chạy: pm2 start ecosystem.config.cjs
 */
module.exports = {
  apps: [
    {
      name: "my-mqtt-app",
      script: "server.js",
      interpreter: "node",
      env: {
        NODE_ENV: "production"
      },
      instances: 1,
      autorestart: true,
      watch: false,
      max_memory_restart: "500M",
      error_file: "./logs/err.log",
      out_file: "./logs/out.log",
      merge_logs: true
    }
  ]
};
