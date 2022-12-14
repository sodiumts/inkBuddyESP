import asyncio
from bleak import BleakClient
from tqdm import tqdm

CHARACTERISTIC_UUID = "b6a5eeef-8f30-46dd-8689-3b874c0fda71"

class ArrayGen():
    def __init__(self,characteristic_uuid) -> None:
        self.mainArray = list()
        self.characteristic = characteristic_uuid

    def listGen(self,type,xStartPos,yStartPos,width,height,data) -> None:
        startingBytes = bytearray([
            xStartPos,
            yStartPos,
            width,
            height,
        ])
        dataConv = bytearray(data,"ascii")
        startingBytes.extend(dataConv)
        self.mainArray.append(startingBytes)
    
    def printGen(self):
        print(self.mainArray)

    async def sender(self):
        async with BleakClient("E81748A0-0237-7885-9A12-AD982257896D") as client:
            for i in tqdm(self.mainArray):
                try:
                    await client.write_gatt_char(CHARACTERISTIC_UUID,i,True) 
                except:
                    print("Finished flashing")
                    quit()

gen = ArrayGen(CHARACTERISTIC_UUID)

gen.listGen(0x0,0x0,0x0,0xFA,0x80,"estere ir susib")
gen.printGen()
asyncio.run(gen.sender())
