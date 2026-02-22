# Dockerfile cho Flask ML App
FROM python:3.11-slim

# Set working directory
WORKDIR /app

# Install system dependencies (nếu cần)
RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements và install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy app code
COPY . .

# Expose port (Railway/Cloud Run sẽ tự động set PORT env)
EXPOSE 5000

# Run với gunicorn
CMD ["gunicorn", "--bind", "0.0.0.0:${PORT:-5000}", "--workers", "2", "--timeout", "120", "app:app"]
