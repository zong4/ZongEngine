import sys

argCount = len(sys.argv)
if (argCount < 2 or argCount > 3):
    print("Usage: ")
    exit()

inputFile = sys.argv[1]
outputFile = "Buffer.embed"
if (argCount == 3):
    outputFile = sys.argv[2]
print(f"Opening {inputFile}")
iterationCount = 0
with open(inputFile, "rb") as i:
    with open(outputFile, "w") as o:

        o.write('const uint8_t g_Buffer[] =\n{\n')

        while (byte := i.read(1)):
            if (iterationCount != 0):
                o.write(', ')
                if (iterationCount % 16 == 0):
                    o.write('\n')

            o.write(f"0x{byte.hex()}")
            iterationCount += 1

        o.write('\n};')

print(f"Wrote {iterationCount} bytes.")