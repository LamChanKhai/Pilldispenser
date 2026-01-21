# ===============================
# TẮT LOG TENSORFLOW (BẮT BUỘC)
# ===============================
import os
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"
os.environ["TF_ENABLE_ONEDNN_OPTS"] = "0"

# ===============================
# IMPORT
# ===============================
import sys
import json
import joblib
import numpy as np
import pandas as pd
from tensorflow.keras.models import load_model

# ===============================
# LOAD MODEL & SCALER
# ===============================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

try:
    model = load_model(os.path.join(BASE_DIR, "hypertension_model.keras"))
    scaler = joblib.load(os.path.join(BASE_DIR, "scaler.pkl"))
    feature_names = joblib.load(os.path.join(BASE_DIR, "feature_names.pkl"))
except Exception as e:
    print(json.dumps({"error": f"Load model failed: {str(e)}"}))
    sys.exit(1)

# ===============================
# NHẬN INPUT JSON
# ===============================
try:
    input_json = json.loads(sys.argv[1])
except Exception:
    print(json.dumps({"error": "Invalid JSON input"}))
    sys.exit(1)

# ===============================
# FEATURE GỐC
# ===============================
num_cols = ['age', 'cigsPerDay', 'sysBP', 'diaBP', 'BMI', 'heartRate']
cat_cols = ['male', 'currentSmoker', 'BPMeds', 'diabetes']

default_values = {
    "male": 0,
    "age": 50,
    "currentSmoker": 0,
    "cigsPerDay": 0,
    "BPMeds": 0,
    "diabetes": 0,
    "sysBP": 120,
    "diaBP": 80,
    "BMI": 25,
    "heartRate": 70
}

# ===============================
# BÙ GIÁ TRỊ THIẾU
# ===============================
for k, v in default_values.items():
    if k not in input_json or input_json[k] in ["", None]:
        input_json[k] = v

# ===============================
# TẠO DATAFRAME
# ===============================
df = pd.DataFrame([input_json])

try:
    df[num_cols] = scaler.transform(df[num_cols])
except Exception as e:
    print(json.dumps({"error": f"Scaler error: {str(e)}"}))
    sys.exit(1)

df = pd.get_dummies(df, columns=cat_cols)

# ===============================
# FIX CỘT THIẾU
# ===============================
for col in feature_names:
    if col not in df.columns:
        df[col] = 0

df = df[feature_names]

# ===============================
# PREDICT
# ===============================
try:
    prob = float(model.predict(df, verbose=0)[0][0])
except Exception as e:
    print(json.dumps({"error": f"Predict error: {str(e)}"}))
    sys.exit(1)

# ===============================
# DIỄN GIẢI KẾT QUẢ
# ===============================
if prob < 0.3:
    bp_level = "Bình thường"
elif prob < 0.6:
    bp_level = "Tiền tăng huyết áp"
else:
    bp_level = "Nguy cơ tăng huyết áp cao"

result = {
    "BP_Level": bp_level,
    "Hypertension_Risk": f"{prob * 100:.2f}%"
}

# ===============================
# CHỈ IN JSON (QUAN TRỌNG)
# ===============================
print(json.dumps(result))
