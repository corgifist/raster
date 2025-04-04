fa_content = open("IconsFontAwesome5.h").read()
fa_lines = fa_content.split("\n")
acc = ""
for line in fa_lines:
    parts = line.split(" ")
    if len(parts) < 2:
        continue
    if parts[0] != '#define':
        continue
    acc += '{"' + parts[1] + '", ' + parts[1] + '},\n'
open("result.cpp", "w+").write(acc)