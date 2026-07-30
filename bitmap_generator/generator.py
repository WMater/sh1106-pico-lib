from PIL import Image

image = Image.open("test2.png")
image = image.convert("1", dither=Image.NONE)
bitmap = image.load();

size = image.size

output = "const uint8_t bitmap[] = {\n0b"
count = 0
for i in range(size[0]):
    
    for j in range(size[1]):
        if bitmap[i, j] == 0:
            output += "0"
        else:
            output += "1"
        count += 1
        print(count)
        if count == 8:
            count = 0
            output += ", 0b"

    if i == size[0] - 1:
        diff = 8 - count
        pad = "0" * diff
        output += pad
        break

    if count == 0:
        output += ",\n0b"
    else:
        diff = 8 - count
        pad = "0" * diff
        output += pad + ",\n0b"
        count = 0

output += "\n};"

struct = "BITMAP bitmap_name = {" + str(size[1]) + ", " + str(size[0]) + ", bitmap};"
out = output + "\n\n\n" + struct
print(out)
image.save("new.png")
image.show()