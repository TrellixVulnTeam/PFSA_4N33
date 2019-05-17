from PIL import Image, ImageFilter, ImageDraw, ImageFont
import pathlib
import os
import sys
def generate(path, textsize):
    path = pathlib.Path(path)
    fnt = ImageFont.truetype('C:\\tmp\\PFSA\\OpenSans-Light.ttf', 100)
    fntb = ImageFont.truetype('C:\\tmp\\PFSA\\OpenSans-Light.ttf', textsize)
    background = Image.open("C:\\tmp\\PFSA\\ben-klea-1593566-unsplash.jpg")
    paddingx = 300
    paddingy = 50
    lineseparation = textsize*2.5
    blur = 1
    print("Step 1 : Cropping")
    background = background.crop((0,0,background.size[0],300+(len(os.listdir(str(path)))*lineseparation)))
    print("Step 2 : Blur")
    for blurcount in range(blur):
        background = background.filter(ImageFilter.BLUR)
    print("Step 3 : Drawing")
    d = ImageDraw.Draw(background)
    d.line((0,90,background.size[0],90), fill=(255,255,255))
    d.text((background.size[0]/2,100), str(path), font=fnt, fill=(255,255,255))
    for x in range(len(os.listdir(str(path)))):
        d.line((0,paddingx+((x)*lineseparation),background.size[0],paddingx+((x)*lineseparation)), fill=(255,255,255))
        d.text((paddingy ,paddingx+((x)*lineseparation)), os.listdir(str(path))[x], font=fntb, fill=(255,255,255))
    return background


