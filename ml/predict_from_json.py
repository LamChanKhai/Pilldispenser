import sys
import json
import joblib
import os
import numpy as np
import pandas as pd

# Lấy thư mục chứa file này
current_dir = os.path.dirname(os.path.abspath(__file__))

# Load các model đã train
try:
    rf_bp = joblib.load(os.path.join(current_dir, "model_bp_level.pkl"))
    rf_risk = joblib.load(os.path.join(current_dir, "model_hypertension_risk.pkl"))
except FileNotFoundError as e:
    print(json.dumps({"error": str(e)}))
    sys.exit(1)

# Nhận dữ liệu từ argv
try:
    input_json = json.loads(sys.argv[1])
except (IndexError, json.JSONDecodeError):
    print(json.dumps({"error": "Cần truyền dữ liệu JSON hợp lệ"}))
    sys.exit(1)

# Danh sách feature theo đúng model train
required_features = [
    "male", "age", "currentSmoker", "cigsPerDay",
    "BPMeds", "diabetes", "totChol", "sysBP",
    "diaBP", "BMI", "heartRate", "glucose"
]

# Giá trị mặc định cho các trường thiếu
mean_values = {
    "male": 0,
    "age": 50,
    "currentSmoker": 0,
    "cigsPerDay": 0,
    "BPMeds": 0,
    "diabetes": 0,
    "totChol": 200,
    "sysBP": 120,
    "diaBP": 80,
    "BMI": 25,
    "heartRate": 70,
    "glucose": 90
}

# 1️⃣ Tự thêm các trường thiếu
for feature in required_features:
    if feature not in input_json or input_json[feature] in ["", None]:
        input_json[feature] = mean_values[feature]

# 2️⃣ Chuyển JSON thành DataFrame
df = pd.DataFrame([input_json])

# 3️⃣ Convert tất cả sang float
try:
    df = df.astype(float)
except Exception as e:
    print(json.dumps({"error": f"Lỗi convert float: {e}"}))
    sys.exit(1)

# 4️⃣ Dự đoán
try:
    bp_level_pred = rf_bp.predict(df)[0]
    risk_prob = rf_risk.predict_proba(df)[0][1] * 100
except Exception as e:
    print(json.dumps({"error": f"Lỗi predict: {e}"}))
    sys.exit(1)

# 5️⃣ Mức huyết áp
bp_levels = ["Bình thường", "Tiền tăng huyết áp", "Tăng huyết áp độ 1", "Tăng huyết áp độ 2"]

# 6️⃣ Trả kết quả JSON
result = {
    "BP_Level": bp_levels[int(bp_level_pred)],
    "Hypertension_Risk": f"{risk_prob:.2f}%"
}

print(json.dumps(result))
