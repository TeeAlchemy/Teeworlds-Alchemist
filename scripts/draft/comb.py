import json

# 从keys.txt文件中读取keys
with open('keys.txt', 'r', encoding='utf-8') as keys_file:
    keys = [line.strip() for line in keys_file.readlines()]

# 从values.txt文件中读取values
with open('values.txt', 'r', encoding='utf-8') as values_file:
    values = [line.strip() for line in values_file.readlines()]

# 确保 keys 和 values 的长度相同
if len(keys) != len(values):
    raise ValueError("The number of keys and values must be the same.")

# 整合keys和values为一个字典列表
combined_data = {'translation': [{'key': key, 'value': value} for key, value in zip(keys, values)]}

# 将整合后的数据保存为JSON文件
with open('combined.json', 'w', encoding='utf-8') as combined_file:
    json.dump(combined_data, combined_file, ensure_ascii=False, indent=4)
