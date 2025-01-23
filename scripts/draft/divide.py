import json

# 从文件中读取JSON数据
with open('a.json', 'r', encoding='utf-8') as file:
    data = json.load(file)

# 提取所有value
values = [item['value'] for item in data['translation']]

# 将提取的values保存到文本文件
with open('values.txt', 'w', encoding='utf-8') as file:
    for value in values:
        file.write(value + '\n')

# 提取所有value
values = [item['key'] for item in data['translation']]

# 将提取的values保存到文本文件
with open('keys.txt', 'w', encoding='utf-8') as file:
    for value in values:
        file.write(value + '\n')
