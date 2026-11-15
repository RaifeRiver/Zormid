import urllib.request
file, headers = urllib.request.urlretrieve("https://www.unicode.org/Public/18.0.0/emoji/emoji-test.txt", "emojis.txt")
with open(file, "r", encoding="utf-8") as file:
    contents = file.read()

json = "{"
for line in contents.splitlines():
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    splitLine = line.split("#", 1)
    type = splitLine[0].split(";")[1].strip()
    if type not in ("fully-qualified", "component"):
        continue
    data = splitLine[1].strip().split(" ")
    name = "_".join(data[2:]).lower()
    name = name.replace(":", "_").replace(".", "_").replace("-", "_").replace("&", "and").replace("“", "").replace("”", "").replace("(", "").replace(")", "").replace("!", "").replace("’", "").replace("*", "asterisk").replace("#", "hash").replace(",", "_")
    while "__" in name:
        name = name.replace("__", "_")
    json += f"\"{name}\":\"{data[0]}\","
json = json[:-1] + "}"
with open("emoji_list.json", "w", encoding="utf-8") as file:
    file.write(json)