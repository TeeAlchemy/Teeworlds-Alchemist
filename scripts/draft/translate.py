import http.client
import hashlib
import urllib
import random
import json

# Baidu fanyi
appid = 'appid'
secretKey = 'key'

target_language = 'ru'  # 西班牙语

with open('values.txt', 'r', encoding='utf-8') as file:
    fields_to_translate = file.read().splitlines()

translated_fields = []

for field in fields_to_translate:
    # 构建请求的URL
    salt = random.randint(32768, 65536)
    sign = appid + field + str(salt) + secretKey
    sign = hashlib.md5(sign.encode()).hexdigest()
    myurl = '/api/trans/vip/translate'
    q = field
    from_lang = 'auto'
    to_lang = target_language
    url = myurl + '?appid=' + appid + '&q=' + urllib.parse.quote(q) + '&from=' + from_lang + '&to=' + to_lang + '&salt=' + str(salt) + '&sign=' + sign

    try:
        httpClient = http.client.HTTPConnection('api.fanyi.baidu.com')
        httpClient.request('GET', url)

        response = httpClient.getresponse()
        result_all = response.read().decode("utf-8")
        result = json.loads(result_all)

        if 'trans_result' in result:
            translated_text = result['trans_result'][0]['dst']
            translated_fields.append(translated_text)
        else:
            print("翻译错误:", result)

    except Exception as e:
        print(e)
    finally:
        if httpClient:
            httpClient.close()
    print(field)

with open('translated_values.txt', 'w', encoding='utf-8') as outfile:
    for translated_field in translated_fields:
        outfile.write(translated_field + '\n')

print("Translation complete. Results saved to 'translated_values.txt'.")
