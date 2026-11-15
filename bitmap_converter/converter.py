from PIL import Image, ImageOps
import os

def get_bits(image, name):

    size = image.size
    bitmap = image.load()

    output = []
    output.append("const uint8_t "+ name + "[] = {\n0b")
    count = 0
    for i in range(size[0]):
    
        for j in range(size[1]):

            if count == 8:
                count = 0
                output.append(", 0b")

            if bitmap[i, j] == 0:
                output.append("0")
            else:
                output.append("1")

            count += 1

        if i == size[0] - 1:
            
            pad = "0" * (8-count)
            output.append(pad)
            break

        pad = "0" * (8 - count)
        output.append(pad + ",\n0b")
        count = 0

    output.append("\n};")
    output.append("\n\nBITMAP bitmap_"+name+" = {" + str(size[1]) + ", " + str(size[0]) + ", " + name + "};")
    return "".join(output)


def main():
    curr_dir = os.getcwd()

    output_dir = os.path.join(curr_dir, "output")
    os.makedirs(output_dir, exist_ok=True)

    files = os.listdir(curr_dir)
    output_file = open(os.path.join(output_dir, "bitmap.h"), "w")
    output_file.write("//custom bitmaps genereted using python script\n\n")
    output_file.write("#ifndef BITMAP_H\n\n#define BITMAP_H\n\n\n")

    for file in files:

        if file.lower().endswith((".png", ".jpg", ".jpeg", ".bmp")):

            try:
                image = Image.open(file)
                image = image.convert("L")
                image = ImageOps.autocontrast(image)
                image = image.convert("1", dither=Image.NONE)

            except OSError:
                print("unable to open file:", file, "file corrupted")
                continue

            except Exception as excpt:
                print("something went wrong with:", file, excpt)
                continue

            bits = get_bits(image, file[:-len(os.path.splitext(file)[1])])
            output_file.write(bits+"\n\n")
            print("converted:", file)
    print("task finished!")
    output_file.write("#endif")
    output_file.close()



if __name__ == "__main__":
    main()
