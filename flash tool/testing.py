import asyncio
from bleak import BleakClient
from tqdm import tqdm
CHARACTERISTIC_UUID = "b6a5eeef-8f30-46dd-8689-3b874c0fda71"

starting = bytearray([0x0,0x0,0xFA,0x80])
string = "ag"
readnumPast = bytearray(string,"ascii")

starting.extend(readnumPast)

def splitter(arr,n):
    for i in range(0,len(arr),n):
        yield arr[i:i +n]
starting = list(splitter(starting,512))

print(starting)

async def main():
    async with BleakClient("E81748A0-0237-7885-9A12-AD982257896D") as client:
        counter = 0
        for i in tqdm(starting):
            try:
                await client.write_gatt_char(CHARACTERISTIC_UUID,i,True) 
                counter +=1
            except:
                print("Finished flashing")
                quit()

asyncio.run(main())