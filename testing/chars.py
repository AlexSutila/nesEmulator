#!/usr/bin/env python3

from PIL import Image
from numpy import tile

'''
Displays the entire 8KiB contents of the CHR memory from the ppu bus dump
    in an easy to digest tile format. Top left tile starts at address $0000
    and moves along in increments of 16 bytes, $0010, $0020, $0030, ... etc
    ...
    Increments top down, left to right 
'''

pal = [0x00,0xFF,0xAA,0x55]
chrRomSize = 0x2000

def readData():

    arr = []
    # Based on the c++ source code, this is the expected location of the dump
    with open("dumps/ppubusdump.txt", "rb") as dump:
        dump.seek(0x0000, 0) # Not really needed but what ever
        for i in range(chrRomSize):
            arr.append(dump.read(1))
    return arr

def main():

    numRows = 0x10 * 8; numCols = 0x10 * 8 * 2;
    pixelsInTile  = 8; bytesInTile = 16
    upscaleFactor = 10

    arr = readData() # Read character data into array
    img = Image.new('RGB', (numRows, numCols))

    curTileX, curTileY = 0, 0
    tileBase = 0x0000

    while tileBase < chrRomSize:
        for i in range(8): # Tile row iteration
            dataLo, dataHi = int.from_bytes(arr[tileBase+i+0],'little'), int.from_bytes(arr[tileBase+i+8],'little')
            for j in range(8): # Tile columns
                color = ((dataLo >> (7 - j)) & 1) | (((dataHi >> (7 - j)) & 1) << 1)
                img.putpixel((curTileX + j, curTileY + i), (pal[color],pal[color],pal[color]))
        # Move the x and y tile indices along
        curTileX += 8
        if curTileX == numRows:
            curTileX, curTileY = 0, curTileY + 8
        # Increment base address for next tile
        tileBase += bytesInTile

    img.resize((numRows * upscaleFactor, numCols * upscaleFactor),resample=Image.NEAREST).show()

if __name__ == "__main__": main()
