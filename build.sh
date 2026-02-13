#!/bin/bash

if [ -z "$1" ]; then
    echo "=== Starting build process ==="
else
    VERSION=$1
    echo "=== Starting build process for $VERSION ==="
fi

echo "Building plugin..."
make clean
make
if [ $? -ne 0 ]; then
    echo "Error: Plugin build failed"
    exit 1
fi
echo "Plugin built successfully"

echo "Building tester..."
cd ./tester
make clean
make
if [ $? -ne 0 ]; then
    echo "Error: Tester build failed"
    exit 1
fi
cd ..
echo "Tester built successfully"

echo ""
echo "=== Copying files ==="
mkdir -p ./bin/build/PSP/GAME/stress-tester
mkdir -p ./bin/build/SEPLUGINS/expover

echo "Tester Files..."
cp -f ./tester/bin/EBOOT.PBP ./bin/build/PSP/GAME/stress-tester/
cp -f ./tester/bin/kcall.prx ./bin/build/PSP/GAME/stress-tester/
cp -f ./tester/README.md ./bin/build/PSP/GAME/stress-tester/
echo "EBOOT.PBP, kcall and README.md copied"

echo "Copying Plugin..."
cp -f ./bin/expover.prx ./bin/build/SEPLUGINS/expover/
cp -f ./README.md ./bin/build/SEPLUGINS/expover/

if [ -z "$1" ]; then
    echo "Copying note.txt..."
    cp -f ./note.txt ./bin/build/SEPLUGINS/expover/
else
    echo "Generating note.txt with version $VERSION..."
    sed "s/{VERSION}/$VERSION/g" ./note.txt.template > ./bin/build/SEPLUGINS/expover/note.txt
fi

echo "expover.prx, README.md and note.txt copied"

echo "Creating overconfig.txt..."
echo "407" > ./bin/build/overconfig.txt
echo "overconfig.txt created"

echo ""
if [ -z "$1" ]; then
    echo "=== Build completed successfully ==="
else
    echo "=== Build completed successfully for $VERSION ==="
fi
