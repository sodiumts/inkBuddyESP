import asyncio
from bleak import BleakClient
from tqdm import tqdm
CHARACTERISTIC_UUID = "5b277b7f-f2e2-438c-9041-a7cda588269c"
readnumPast = bytearray()
size = 512
with open("ble.bin","rb") as f:
    mainFile = bytearray(f.read())

def splitter(arr,n):
    for i in range(0,len(arr),n):
        yield arr[i:i +n]
arrayOfData = list(splitter(mainFile,size))




async def main():
    async with BleakClient("E81748A0-0237-7885-9A12-AD982257896D") as client:
        counter = 0
        for i in tqdm(arrayOfData):
            try:
                await client.write_gatt_char(CHARACTERISTIC_UUID,i,True) 
                counter +=1
            except:
                print("Finished flashing")
                quit()

asyncio.run(main())