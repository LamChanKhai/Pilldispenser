import pandas as pd
import numpy as np

# =========================
# 1. ĐỌC FILE DỮ LIỆU
# =========================
input_file = "dataset/processed_dataset.csv"        # file gốc
output_file = "dataset/statistics.csv" # file xuất ra

df = pd.read_csv(input_file)

# =========================
# 2. XÁC ĐỊNH LOẠI DỮ LIỆU
# =========================
def detect_type(series):
    unique_vals = series.dropna().unique()
    if set(unique_vals).issubset({0, 1}):
        return "binary"
    return "numeric"

# =========================
# 3. TÍNH MEAN & STD
# =========================
results = []

for col in df.columns:
    if pd.api.types.is_numeric_dtype(df[col]):
        mean = df[col].mean()
        std = df[col].std()

        results.append({
            "giá trị": col,
            "loại dữ liệu": detect_type(df[col]),
            "giá trị trung bình": round(mean, 4),
            "độ lệch chuẩn": round(std, 4)
        })

# =========================
# 4. XUẤT FILE
# =========================
result_df = pd.DataFrame(results)
result_df.to_csv(output_file, index=False, encoding="utf-8-sig")

print("✅ Đã xuất file:", output_file)
