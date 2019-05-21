from PIL import Image, ImageFilter, ImageDraw, ImageFont
import pathlib
import os
import sys
def generate(path, textsize):
    path = pathlib.Path(path)
    fnt = ImageFont.truetype('C:\\tmp\\PFSA\\font.ttf', 100)
    fntb = ImageFont.truetype('C:\\tmp\\PFSA\\font.ttf', textsize)
    background = Image.open("C:\\tmp\\PFSA\\background.jpg")
    paddingx = 300
    paddingy = 50
    lineseparation = textsize*2.5
    blur = 1
    print("Step 2 : Cropping")
    background = background.crop((0,0,background.size[0],300+(len(os.listdir(str(path)))*lineseparation)))
    print("Step 3 : Blur")
    for blurcount in range(blur):
        background = background.filter(ImageFilter.BLUR)
    print("Step 4 : Drawing")
    d = ImageDraw.Draw(background)
    offset = fnt.getoffset(str(path))
    d.line((0,90,background.size[0],90), fill=(255,255,255))
    if len(str(path)) >= 70:
        fnt = ImageFont.truetype('C:\\tmp\\PFSA\\font.ttf', round(100/(len(str(path))/75)))
    d.text((paddingy*2,125),str(path), font=fnt, fill=(255,255,255))
    for x in range(len(os.listdir(str(path)))):
        d.line((0,paddingx+((x)*lineseparation),background.size[0],paddingx+((x)*lineseparation)), fill=(255,255,255))
        d.text((paddingy ,paddingx+((x)*lineseparation)), os.listdir(str(path))[x], font=fntb, fill=(255,255,255))
    return background


